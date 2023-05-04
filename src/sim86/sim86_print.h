struct instr;
struct state;

// Print instruction disassembly to stdout
void print_instr(const struct instr *instr);

// Print diff between 2 simulation states to stdout
void print_state_diff(const struct state *old_state,
    const struct state *new_state);

// Dump state of all simulation state registers to stdout
void print_state_registers(const struct state *state);
