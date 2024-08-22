#include <stdio.h>

#include "kluski.h"

char * cmdnames[] = { "open", "argv", "func", "retv"} ;
char * regnames[] = { "x8", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"} ; // NOTE: first reg in this list is for function pointer (not actually used; see 'bl')
char * retnames[] = { "x9", "x10", "x11", "x12", "x13", "x14", "x15" } ; // return value appears in x0

void emit_start(FILE * out) {
//    printf(".text\n");
    fprintf(out, ".globl _times\n");
    // This is a demonstration of a primitive written in assembly;
    // we also link against primitives written in C.
    fprintf(out, "_times:\n");
    fprintf(out, "    mul x0, %s, %s /* direct mul into return reg  */\n", regnames[1], regnames[2]);
    fprintf(out, "    ret\n");

    fprintf(out, ".globl _main\n");
    fprintf(out, "_main:\n");
    fprintf(out, "    stp fp, lr, [sp, #-0x10]!\n"); // save frame pointer and link register as nested function calls will overwrite it
}

void emit_code(FILE * out, CtsEntry * entry, int pos, int num_args) {
    if (pos >= sizeof(regnames) / sizeof(char *)) { printf("Too many arguments.\n"); return; }
    switch(entry->command) {
        case CTS_FUNC:
            switch(entry->value) {
                case '+':
                    // X0 already carries first arg, so that's convenient
                    for (int i=2; i<num_args; i++) {
                        fprintf (out, "    add x0, x0, %s /* add argument # %d */ \n", regnames[i], i);
                    }
                    break;
                case '*':
                    if (pos == 0) { // that's the function position; in any other position, function == common argument
                        fprintf (out, "    bl _times\n");
                    } else {
                        // Apparently we're only pointing to the function
                        fprintf (out, "    adr %s, _times\n", regnames[pos]); // That's for function pointers
                    }
                    break;
                case '?':
                    if (pos == 0) { // that's the function position; in any other position, function == common argument
                        fprintf (out, "    bl _printnum\n");//, regnames[0]);
                    } else {
                        // Apparently we're only pointing to the function
                        fprintf (out, "    adr %s, _printnum\n", regnames[pos]); // That's for function pointers
                    }
                    break;
            }
            break;
        case CTS_ARGV:
            fprintf (out, "    mov %s, %d\n", regnames[pos], entry->value);
            break;
        case CTS_RETV:
            fprintf (out, "    mov %s, %s    /* enlist previous return value */\n", regnames[pos], retnames[entry->value]);
            break;
        default:
            fprintf (out, "Unexpected command\n");
            break;
    }
}

void emit_save_retval(FILE * out, int n) {
    if (n >= sizeof(retnames) / sizeof(char *)) { printf("Too many return values.\n"); return; }
    fprintf(out, "    mov %s, x0    /* preserve return value        */\n", retnames[n]);
}

void emit_end(FILE * out) {
    fprintf(out, "    ldp fp, lr, [sp], #0x10\n"); // restore frame pointer and link register
    fprintf(out, "    ret\n");
}
