#include <stdio.h>

#include "kluski.h"

char * cmdnames[] = { "open", "argv", "func", "retv"} ;
char * regnames[] = { "%rax", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"} ; // NOTE: first reg in this list is for function pointer (= same as return value reg)
char * retnames[] = { "%r10", "%r11", "%r12", "%r13", "%r14", "%r15" } ; // return value appears in %rax; then we copy them to one of these. NOTE: r12 and up should be callee saved!

void emit_start() {
//    printf(".text\n");
    printf(".globl times\n");
    // This is a demonstration of a primitive written in assembly;
    // we also link against primitives written in C.
    printf("times:\n");
    printf("    mov %s, %%rax    /* move to return reg           */\n", regnames[1]);
    printf("    mul %s   /* direct multiply (rax is implicit)   */\n", regnames[2]);
    printf("    ret\n");
    printf(".globl main\n");
    printf("main:\n");
}

void emit_code(CtsEntry * entry, int pos) {
    if (pos >= sizeof(regnames) / sizeof(char *)) { printf("Too many arguments.\n"); return; }
    switch(entry->command) {
        case CTS_FUNC:
            switch(entry->value) {
                case '+':
                    printf("    mov %s, %%rax    /* move to return reg           */\n", regnames[1]);
                    printf("    add %s, %%rax    /* direct add                   */\n", regnames[2]);
                    break;
                case '*':
                    printf ("    lea times(%%rip), %s\n", regnames[pos]); // That's for function pointers
                    if (pos == 0) { // that's the function position; in any other position, function == common argument
                        printf ("    call *%s\n", regnames[0]);
                    }
                    break;
                case '?':
                    printf ("    lea printnum(%%rip), %s\n", regnames[pos]); // That's for function pointers
                    if (pos == 0) { // that's the function position; in any other position, function == common argument
                        printf ("    call *%s\n", regnames[0]);
                    }
                    break;
            }
            break;
        case CTS_ARGV:
            printf ("    mov $%d, %s\n", entry->value, regnames[pos]);
            break;
        case CTS_RETV:
            printf ("    mov %s, %s    /* enlist previous return value */\n", retnames[entry->value], regnames[pos]);
            break;
        default:
            printf("Unexpected command\n");
            break;
    }
}

void emit_save_retval(int n) {
    if (n >= sizeof(retnames) / sizeof(char *)) { printf("Too many return values.\n"); return; }
    printf("    mov %%rax, %s    /* preserve return value        */\n", retnames[n]);
}

void emit_end() {
    printf("    ret\n");
    printf(".section        .note.GNU-stack,\"\",@progbits\n");
}

