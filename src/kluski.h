typedef enum CtsCommand {
    CTS_OPEN, // Open expression (bracket)
    CTS_ARGV, // Add argument value to current expression
    CTS_FUNC, // Built-in function reference
    CTS_RETV  // Add return value as argument to current expression
} CtsCommand;

extern char * cmdnames[];

typedef struct CtsEntry {
    enum CtsCommand command;
    unsigned int value;
} CtsEntry;

typedef struct CtStack {
    unsigned int size;
    unsigned int length;
    struct CtsEntry * values;
} CtStack;

#define cts_push(cts, fn, val) (cts)->values[(cts)->length++] = (CtsEntry) {fn, val}
#define cts_pop(cts) &((cts)->values[--(cts)->length])

extern char * regnames[];

void emit_start();
void emit_code(CtsEntry * entry, int pos);
void emit_save_retval(int n);
void emit_end();

