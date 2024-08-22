#include <stdio.h>

#include "kluski.h"

char * cmdnames[] = { "open", "argv", "func", "retv"} ;
char * regnames[] = { "%rax", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"} ; // NOTE: first reg in this list is for function pointer (= same as return value reg)
char * retnames[] = { "%r10", "%r11", "%r12", "%r13", "%r14", "%r15" } ; // return value appears in %rax; then we copy them to one of these. NOTE: r12 and up should be callee saved!

void emit_start(FILE * out) {
//    printf(".text\n");
    fprintf(out, ".globl times\n");
    // This is a demonstration of a primitive written in assembly;
    // we also link against primitives written in C.
    fprintf(out, "times:\n");
    fprintf(out, "    mov %s, %%rax    /* move to return reg           */\n", regnames[1]);
    fprintf(out, "    mul %s     /* direct multiply (rax is implicit) */\n", regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, ".globl main\n");
    fprintf(out, "main:\n");
}

void emit_code(FILE * out, CtsEntry * entry, int pos, int num_args) {
    if (pos >= sizeof(regnames) / sizeof(char *)) { printf("Too many arguments.\n"); return; }
    switch(entry->command) {
        case CTS_FUNC:
            switch(entry->value) {
                case 0: // '+'; this may get more generalized still
                    fprintf(out, "    mov %s, %%rax    /* move to return reg           */\n", regnames[1]);
                    for (int i=2; i<num_args; i++) {
                        fprintf(out, "    add %s, %%rax    /* direct add                   */\n", regnames[i]);
                    }
                    break;
                case 1: // '*'; apart from the function call we presently make, this operator requires a different expression form from '+' etc. on x86
                    fprintf(out, "    lea times(%%rip), %s\n", regnames[pos]); // That's for function pointers
                    if (pos == 0) { // that's the function position; in any other position, function == common argument
                        fprintf(out, "    call *%s\n", regnames[0]);
                    }
                    break;
                default: // print, ...
                    fprintf(out, "    lea %s(%%rip), %s\n", primitive_names[entry->value], regnames[pos]); // That's for function pointers
                    if (pos == 0) { // that's the function position; in any other position, function == common argument
                        fprintf(out, "    call *%s\n", regnames[0]);
                    }
                    break;
            }
            break;
        case CTS_ARGV:
            fprintf(out, "    mov $%d, %s\n", entry->value, regnames[pos]);
            break;
        case CTS_RETV:
            fprintf(out, "    mov %s, %s    /* enlist previous return value */\n", retnames[entry->value], regnames[pos]);
            break;
        default:
            printf("Unexpected command\n");
            break;
    }
}

void emit_save_retval(FILE * out, int n) {
    if (n >= sizeof(retnames) / sizeof(char *)) { printf("Too many return values.\n"); return; }
    fprintf(out, "    mov %%rax, %s    /* preserve return value        */\n", retnames[n]);
}

void emit_end(FILE * out) {
    fprintf(out, "    ret\n");
    fprintf(out, ".section        .note.GNU-stack,\"\",@progbits\n");
}

