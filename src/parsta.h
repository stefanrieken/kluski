typedef enum ParseStackType {
    PT_INT,
    PT_STR,
    PT_FUN,
    PT_REF,
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

extern char * primitive_names[];
# define num_primitives (sizeof(primitive_names) / sizeof(char *))

// The pointer to the 'top' of defined variables
extern int stashptr;

// Defined overall
int emit_code(FILE * out, ParseStack * stack, int from, char until);
int skip_until_close(ParseStack * stack, int from);
// Defined per platform
void emit_start(FILE * out);
int emit_entry(FILE * out, ParseStack * stack, int from, int n_arg, int n_args, int * stashbase);
void emit_block(FILE * out, int pos, int n_blocks);
void emit_save_retval(FILE * out);
void emit_end(FILE * out);

