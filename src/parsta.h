#include <stdint.h>

typedef enum ParseStackType {
    PT_INT,
    PT_STR,
    PT_FUN,
//    PT_REF, // instead we insert (get "x")
    PT_OPN,
    PT_CLS
} ParseStackType;

typedef struct ParseStackEntry {
    enum ParseStackType type;
    union {
        int num;
        char * str;
    } value;
} ParseStackEntry;

typedef struct ParseStack {
    int size;
    int length;
    ParseStackEntry * entries;
} ParseStack;

typedef struct StringEntry {
    struct StringEntry * next;
    char * str;
} StringEntry;
extern StringEntry * unique_strings;

extern char * primitive_names[];
//# define num_primitives (sizeof(primitive_names) / sizeof(char *))
extern int num_primitives;

// The pointer to the 'top' of defined variables
extern int stashptr;

extern int block_depth;

// Defined overall
void emit_strings(FILE * out);
int emit_code(FILE * out, ParseStack * stack, int from, char until);
int skip_until_close(ParseStack * stack, int from);
int num_args(ParseStack * stack, int from);

// Defined per platform
void emit_start(FILE * out);
int emit_entry(FILE * out, ParseStack * stack, int from, int n_arg, int n_args, int * stashbase);
void emit_save_retval(FILE * out);
void emit_move_retval(FILE * out, int n_arg);
void emit_call_subexpr(FILE * out);
void emit_end(FILE * out);

