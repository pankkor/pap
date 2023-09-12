#pragma once

#include "types.h"

struct os {
#if _WIN32
#elif __APPLE__
#else
  i32 pe_page_fault_fd; // perf event: page faults
  b32 is_initialized;
#endif // #if _WIN32
};

// Initialize and start performance events monitoring.
b32 os_init(void);

// Read Memory Page Faults counter for this process
u64 os_read_page_fault_count(void);
