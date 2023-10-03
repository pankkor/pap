#include "os.h"

#if _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>              // OpenProces VirtualAlloc VirtualFree

#endif // #if _WIN32

// --------------------------------------
// Perf counters
// --------------------------------------

#if _WIN32
#include <psapi.h>                // GetProcessMemoryInfo GetCurrentProcessId

struct os_metrics {
  HANDLE process_handle;
  b32 is_initialized;
};

struct os s_os;

b32 os_init(void)
{
  if(!s_os.is_initialized) {
    s_os.process_handle = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        GetCurrentProcessId());
    s.os.is_initialized = s_os.process_handle != 0;
  }
  return s_os.is_initialized;
}

u64 os_read_page_fault_count(void) {
  PROCESS_MEMORY_COUNTERS_EX pmc = {
    .cb = sizeof(pmc);
  };
  GetProcessMemoryInfo(s_os.process_handle,
      (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
  return pmc.PageFaultCount;
}

u64 os_get_page_size(void) {
  // TODO not implemented
  return 4 * 1024 * 1024;
}

#elif __APPLE__

#include <sys/resource.h>         // getrusage
#include <unistd.h>               // getpagesize

b32 os_init(void) {
  return true;
}

u64 os_read_page_fault_count(void) {
  // NOTE: ru_minflt  page faults serviced without any I/O activity.
  //       ru_majflt  page faults serviced that required I/O activity.
  struct rusage rusage = {0};
  getrusage(RUSAGE_SELF, &rusage);
  return rusage.ru_minflt + rusage.ru_majflt;
}

u64 os_get_page_size(void) {
  return getpagesize();
}

#else

#include <linux/hw_breakpoint.h>  // HW_*
#include <linux/perf_event.h>     // PERF_*
#include <sys/ioctl.h>            // ioctl
#include <sys/syscall.h>          // SYS_*
#include <sys/types.h>            // pid_t
#include <unistd.h>               // syscall read getpagesize

struct os {
  i32 pe_page_fault_fd; // perf event: page faults
  b32 is_initialized;
};

struct os s_os;

// man perf_event_open
static i32 perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
    i32 cpu, i32 group_fd, u64 flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

b32 os_init(void) {
  if (!s_os.is_initialized) {
    struct perf_event_attr pf_attr = {
      .size = sizeof(pf_attr),
      .type = PERF_TYPE_SOFTWARE,
      .config = PERF_COUNT_SW_PAGE_FAULTS,
      .disabled = 0,
      .exclude_kernel = 1,
      .exclude_hv = 1,
    };

    i32 pf_fd = perf_event_open(
        &pf_attr,
        0, -1,  // pid == 0, cpu == -1: measure *this* process on *all* CPUs
        -1,     // group leader
        PERF_FLAG_FD_CLOEXEC);

    s_os.pe_page_fault_fd = pf_fd;
    s_os.is_initialized = pf_fd != -1;
  }

  return s_os.is_initialized;
}

u64 os_read_page_fault_count(void) {
  u64 ret = -1;
  if (s_os.pe_page_fault_fd != -1) {
    if (read(s_os.pe_page_fault_fd, &ret, sizeof(ret)) == -1) {
      ret = -1;
    }
  }
  return ret;
}

u64 os_get_page_size(void) {
  return getpagesize();
}

#endif // #if _WIN32

// --------------------------------------
// Virtual memory
// --------------------------------------

#if _WIN32

void *os_virtual_alloc(u64 size) {
  return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

b32 os_virtual_free(void *p, u64 size) {
  return VirtualFree(p, size, MEM_RELEASE);
}

b32 os_virtual_lock(void *p, u64 size) {
  // TODO implement me
  return false;
}

b32 os_virtual_unlock(void *p, u64 size) {
  // TODO implement me
  return false;
}

#else

#include <sys/mman.h>             // mmap munmap mlock munlock

void *os_virtual_alloc(u64 size) {
  return mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

b32 os_virtual_free(void *p, u64 size) {
  return munmap(p, size) != -1;
}

b32 os_virtual_lock(void *p, u64 size) {
  return mlock(p, size) != -1;
}

b32 os_virtual_unlock(void *p, u64 size) {
  return munlock(p, size) != -1;
}

#endif // #if _WIN32
