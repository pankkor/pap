struct instr;
struct state;

// Print instruction disassembly to stdout
void print_instr(const struct instr *instr);

// Print diff between 2 simulation states to stdout
void print_state_diff(const struct state *state_old, const struct state *state_new);

// Dump state of all simulation state registers to stdout
void print_state_registers(const struct state *state);
