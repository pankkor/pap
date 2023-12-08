#include "os.h"

#include <assert.h>               // assert
#include <stdio.h>                // fprintf perror

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

b32 os_perf_init(void)
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

b32 os_perf_init(void) {
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

b32 os_perf_init(void) {
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

static u64 align(u64 v, u64 alignment) {
  return (v + alignment -1) & ~(alignment - 1);
}

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

// TODO: not tested
void os_print_last_error(const char *msg) {
  if (msg) {
    fprintf(stderr, "%s: ", msg);
  }

  DWORD error = GetLastError();
  TCHAR *system_error_msg;
  b32 res = FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
      | FORMAT_MESSAGE_IGNORE_INSERTS,
      0,
      errCode,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
      (LPTSTR)&&msg,
      0,
      0))

  if (res) {
    _tfprintf(stderr, _T("%s\n"), system_error_msg);
    LocalFree(system_error_msg);
  } else {
    fprintf(stderr, "<unknown>\n");
  }
}

u64 os_file_size_bytes(const char *filepath) {
  struct __stat64 st;
  int err = _stat64(filepath, &st);
  return err ? 0 : st.st_size;
}

struct os_buf os_file_mmap(const char *filepath) {
  struct os_buf ret = {0};

  HANDLE map;
  HANDLE file = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

  if (file != INVALID_HANDLE_VALUE) {
    u64 size = GetFileSize(file, 0);
    if (size != INVALID_FILE_SIZE && size != 0) {
      map = CreateFileMappingA(file, 0, PAGE_READONLY, 0, size, 0);
      if (map) {
        void *map = (void *)MapViewOfFile(map, FILE_MAP_READ, 0, 0, size);
        CloseHandle(map);

        ret.data = map;
        ret.size = size;
      }
    }
    CloseHandle(file);
  }

  return ret;
}

b32 os_file_munmap(struct os_buf buf) {
  return UnmapViewOfFile(buf.data);
}

#else

#include <sys/stat.h>             // stat
#include <sys/mman.h>             // mmap munmap mlock munlock
#include <fcntl.h>                // open

#if __APPLE__
#include <mach/mach_vm.h>
#else
#include <linux/mman.h>           // MAP_HUGE_2MB
#endif // #if __APPLE__

struct os_validator g_os_validator;

// TODO: Transform to FILE + LINE macro
static void validator_error(const char *error) {
  if (g_os_validator.log_error) {
    g_os_validator.log_error(error);
  }

  if (g_os_validator.trap_on_error) {
    __builtin_trap();
  }
}

// NOTE: VirtualAlloc() returns 0 on failure and mmap() returns -1.
// 0 is a valid mmap address, but it's easier to test for 0 in user code,
// therefore we use it.
static void *remap_mmap_failure_to_zero(void *m) {
  if (!m) {
    validator_error(
        "[OS] Error: mmap returned address 0, which is valid for mmap, "
        "but os_ module API uses zero to indicate a failure.\n");
  }

  return m == MAP_FAILED ? 0 : m;
}

void *os_virtual_alloc(u64 size) {
  void *m;
  int flags = MAP_PRIVATE | MAP_ANON;
#if __linux__
  // NOTE: MAP_POPULATE prefaults pages, only supported on Linux
  flags |= MAP_POPULATE;
#endif // #if __linux__
  m = mmap(0, size, PROT_READ | PROT_WRITE, flags, -1, 0);
  m = remap_mmap_failure_to_zero(m);
  return m;
}

void (*s_log_error)(const char *) = 0;

void *os_virtual_large_alloc(u64 *out_size) {
  // Ensure allocated size is multiple of 2MB page size
  // TODO: Linux has support for 1GB page sizes
  u64 size = align(*out_size, 2 * 1024 * 1024);

  void *m;
#if __APPLE__
#ifndef __x86_64__
  // XNU has superpage support only on x86_64
  validator_error("[OS] Error: only x86_64 macOS supports superpages!\n");
#endif // #ifndef __x86_64__
  m = mmap(0, size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANON, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0);
#else
  // NOTE: MAP_POPULATE prefaults pages, only supported on Linux
  m = mmap(0, size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANON | MAP_HUGETLB | MAP_POPULATE | MAP_HUGE_2MB,
      -1, 0);
#endif // #if __APPLE__
  m = remap_mmap_failure_to_zero(m);
  if (m) {
    *out_size = size;
  }
  return m;
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

void os_print_last_error(const char *msg) {
  perror(msg);
}

u64 os_file_size_bytes(const char *filepath) {
  struct stat st;
  int err = stat(filepath, &st);
  return err ? 0 : st.st_size;
}

struct os_buf os_file_mmap(const char *filepath) {
  struct os_buf ret = {0};

  int fd = open(filepath, O_RDONLY);
  if (fd != -1) {
    struct stat st;
    if (!fstat(fd, &st)) {
      if (st.st_size > 0) {
        void *m = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        m = remap_mmap_failure_to_zero(m);
        ret.data = m;
        ret.size = st.st_size;
      }
    }
    close(fd);
  }

  return ret;
}

b32 os_file_munmap(struct os_buf buf) {
  return munmap(buf.data, buf.size) != -1;
}

#endif // #if _WIN32
