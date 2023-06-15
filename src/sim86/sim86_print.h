#include "sim86_types.h"
#include "sim86_clocks.h"

struct instr;
struct state;

// Print instruction disassembly to stdout
void print_instr(const struct instr *instr);

// Print diff between 2 simulation states to stdout
// Set 'skip_ip' to true to omit printing changes to IP register
void print_state_diff(const struct state *old_state,
    const struct state *new_state, bool skip_ip);

// Dump state of all simulation state registers to stdout
// Set 'skip_ip' to true to omit printing changes to IP register
void print_state_registers(const struct state *state, bool skip_ip);

// Print clocks estiamtion, with a total counter cycles
void print_clocks(struct clocks clocks, u32 total);
