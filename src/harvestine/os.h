#pragma once

#include "types.h"

// --------------------------------------
// Perf counters
// --------------------------------------

// Initialize and start performance events monitoring.
b32 os_perf_init(void);

// Read Memory Page Faults counter for this process.
u64 os_read_page_fault_count(void);

// Get page size
u64 os_get_page_size(void);

// --------------------------------------
// Virtual memory
// --------------------------------------

// Reserve virtual memory of size in bytes, no physical pages are allocated.
// Returns 0 on failure.
void *os_virtual_alloc(u64 size);

// Reserve virtual memory of size in bytes, no physical pages are allocated.
// Use Large Pages (aka Linux: Huge Pages, maxOS: Super Pages)
// Writes allocated size multiple of to page size back to `out_size`
// Returns 0 on failure.
void *os_virtual_large_alloc(u64 *out_size);

u64 os_get_large_page_size(void);

// Release virtual memory of size in bytes.
// It's necessary to pass the size according to page size alignment
// Pass the size that was returned to you by os_virtual_lorge_alloc()
// Returns false on failure.
b32 os_virtual_free(void *p, u64 size);

// Lock virtual address in physical memory.
// Returns false on failure.
b32 os_virtual_lock(void *p, u64 size);

// Unlock virtual address from physical memory.
// Returns false on failure.
b32 os_virtual_unlock(void *p, u64 size);

// --------------------------------------
// File System
// --------------------------------------

// Return 64-bit size of a file with filename, by issuing stat syscall.
// Returns 0 on failure.
u64 os_file_size_bytes(const char *filepath);

struct os_buf {
  void *data;
  u64 size;
};

// Return file memory map.
// Returns data = 0 and size = 0 on failure or if file is empty.
struct os_buf os_file_mmap(const char *filepath);

// Unmap file memory map, previously created with os_file_map().
// Returns false on failure.
b32 os_file_munmap(struct os_buf buf);

// Validator logs errors and traps on errors
struct os_validator {
  int (*log_error)(const char *); // puts wors just fine for now
  b32 trap_on_error;
};

extern struct os_validator g_os_validator;
