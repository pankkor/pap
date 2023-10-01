#include "os.h"

struct os {
#if _WIN32
#elif __APPLE__
#else
  i32 pe_page_fault_fd; // perf event: page faults
#endif // #if _WIN32
  b32 is_initialized;
};

#if _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

u64 os_read_page_fault_count(void) {
  return -1;
}

#elif __APPLE__

#include <sys/resource.h> // getrusage

b32 os_init(void) {
  return true;
}

u64 os_read_page_fault_count(void) {
  // NOTE: ru_minflt  the number of page faults serviced without any I/O activity.
  //       ru_majflt  the number of page faults serviced that required I/O activity.
  struct rusage rusage = {0};
  getrusage(RUSAGE_SELF, &rusage);
  return rusage.ru_minflt;
}

#else

#include <linux/hw_breakpoint.h>  // HW_*
#include <linux/perf_event.h>     // PERF_*
#include <sys/ioctl.h>            // ioctl
#include <sys/syscall.h>          // SYS_*
#include <sys/types.h>            // pid_t
#include <unistd.h>               // syscall read

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

#endif // #if _WIN32
