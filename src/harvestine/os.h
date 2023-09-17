#pragma once

#include "types.h"

// Initialize and start performance events monitoring.
b32 os_init(void);

// Read Memory Page Faults counter for this process
u64 os_read_page_fault_count(void);
