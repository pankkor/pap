#pragma once

#include "types.h"

// Initialize and start performance events monitoring.
b32 os_init(void);

// Read Memory Page Faults counter for this process.
u64 os_read_page_fault_count(void);

// Reserve virtual memory of size in bytes, no physical pages are allocated.
// Returns 0 on failure.
void *os_virtual_alloc(u64 size);

// Release virtual memory of size in bytes.
// Returns false on failure.
b32 os_virtual_free(void *p, u64 size);

// Lock virtual address in physical memory.
// Returns false on failure.
b32 os_virtual_lock(void *p, u64 size);

// Unlock virtual address from physical memory.
// Returns false on failure.
b32 os_virtual_unlock(void *p, u64 size);
