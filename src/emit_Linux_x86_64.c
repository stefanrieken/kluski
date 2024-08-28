#include <stdio.h>

//#include "kluski.h"
#include "parsta.h"

char * cmdnames[] = { "add", "mul", "remainder", "blr"};
char * regnames[] = { "%rax", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"} ; // NOTE: first reg in this list is for function pointer (= same as return value reg)
char * retnames[] = { "%r10", "%r11", "%r12", "%r13", "%r14", "%r15" } ; // return value appears in %rax; then we copy them to one of these. NOTE: r12 and up should be callee saved!

int num_regnames = sizeof(regnames) / sizeof(char *);

void emit_start(FILE * out) {
//    printf(".text\n");
    fprintf(out, ".globl times\n");
    // This is a demonstration of a primitive written in assembly;
    // we also link against primitives written in C.
    fprintf(out, "add:\n");
    fprintf(out, "    mov %s, %%rax     /* move to return reg         */\n", regnames[1]);
    fprintf(out, "    add %s, %%rax\n", regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, "mul:\n");
    fprintf(out, "    mov %s, %%rax     /* move to return reg         */\n", regnames[1]);
    fprintf(out, "    mul %s     /* direct multiply (rax is implicit) */\n", regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, ".globl eval\n");
    fprintf(out, "remainder:\n");
    fprintf(out, "    mov %s, %%rax     /* dividend in return reg     */\n", regnames[1]);
    fprintf(out, "    div %s            /* rax = result; rdx = rest   */\n", regnames[2]);
    fprintf(out, "    mov %%rdx, %%rax  /* return remainder           */\n");
    fprintf(out, "    ret\n");
    fprintf(out, "funcall:               /* (demo) function ptr support    */\n"); // 'funcal' _is_ a 'C primitive'
    for (int i=1; i<num_regnames; i++) {
        fprintf(out, "    mov %s, %s\n", regnames[i], regnames[i-1]);
    }
    fprintf(out, "    call *%s\n", regnames[0]); // TODO jump instead of call
    fprintf(out, "    ret\n");
    fprintf(out, "eval:\n");
    fprintf(out, "    call *%s\n", regnames[1]); // TODO jump instead of call
    fprintf(out, "    ret\n");
    fprintf(out, ".globl main\n");
    fprintf(out, "main:\n");
}

int emit_entry(FILE * out, ParseStack * stack, int from, int n_arg, int n_args, int * stashbase) {
    ParseStackEntry * entry = &(stack->entries[from]);
    switch(entry->type) {
        case PT_INT:
            fprintf(out, "    mov $%d, %s\n", entry->value.num, regnames[n_arg]);
            break;
        case PT_STR:
            int idx = 0;
            StringEntry * e = unique_strings;
            while(e != NULL) { if (e->str == entry->value.str) break;  e = e->next; idx++; }
            fprintf(out, "    lea str%d(%%rip), %s\n", idx, regnames[n_arg]);
            break;
        case PT_FUN:
            switch(entry->value.num) {
                case 0: // '+' and friends
                    if (n_arg == 0) { // that's the function position; in any other position, function == common argument
                        fprintf(out, "    mov %s, %%rax     /* move to return reg          */\n", regnames[1]);
		        for (int i=2; i<n_args; i++) {
		            fprintf(out, "    %s %s, %%rax     /* direct %s                  */\n", cmdnames[entry->value.num], regnames[i], cmdnames[entry->value.num]);
		        }
                    } else {
                        fprintf(out, "    lea %s(%%rip), %s\n", cmdnames[entry->value.num], regnames[n_arg]);
                    }
                    break;
                case 1: // '*'; apart from the function call we presently make, this operator requires a different expression form from '+' etc. on x86
                case 2: // '%'
                    fprintf(out, "    lea %s(%%rip), %s\n", cmdnames[entry->value.num], regnames[n_arg]); // That's for function pointers
                    if (n_arg == 0) { // that's the function position; in any other position, function == common argument
                        fprintf(out, "    call *%s\n", regnames[0]);
                    }
                    break;
                default: // "funcall", "printnum", "print", ...
                    fprintf(out, "    lea %s(%%rip), %s\n", primitive_names[entry->value.num], regnames[n_arg]); // That's for function pointers
                    if (n_arg == 0) { // that's the function position; in any other position, function == common argument
                        fprintf(out, "    call *%s\n", regnames[0]);
                    }
                    break;
            }
            break;  
        case PT_REF:
            printf("    TODO ref\n");
            break;  
        case PT_OPN:
            if (entry->value.num == '(') {
                if (*stashbase != stashptr) // Last subexpr was not stashed
                    fprintf(out, "    mov %s, %s     /* unstash return value        */\n", retnames[(*stashbase)++], regnames[n_arg]);
                // Skip sub-expression since it was already written
                return skip_until_close(stack, from)-1;
            } else {
                block_depth++;
 //               printf("    store and jump over block\n");
                fprintf(out, "    lea 0f(%%rip), %s /* load start of block as arg  */\n", regnames[n_arg]);
                fprintf(out, "    jmp %df             /* jump over the block         */\n", block_depth);
                fprintf(out, "0:                     /* start of block              */\n");
                from = emit_code(out, stack, from+1, '}')-1;
                fprintf(out, "    ret                /* return from block */\n");
                fprintf(out, "%d:\n", block_depth);
                block_depth--;
                return from;
            }
        case PT_CLS:
            // Expecting caller to halt expression at CLS
            // without calling us (even in case of ';')
            printf("Error: bracket mismatch\n");
            break;
    }

    return from;
}


void emit_save_retval(FILE * out) {
    if (stashptr >= sizeof(retnames) / sizeof(char *)) { printf("Too many return values.\n"); return; }
    fprintf(out, "    mov %%rax, %s     /* stash return value          */\n", retnames[stashptr++]);
}

void emit_move_retval(FILE * out, int n_arg) {
    fprintf(out, "    mov %s, %s          /* transfer return value to arg   */\n", regnames[0], regnames[n_arg]);
}

void emit_block(FILE * out, int pos, int label) {
    fprintf(out, "    lea 0f(%%rip), %s /* load start of block as argument */\n", regnames[pos]);
    fprintf(out, "    jmp %df           /* jump over the block            */\n", label);
    fprintf(out, "0:                    /* start of block                  */\n");
}

void emit_block_end(FILE * out, int label) {
    fprintf(out, "ret                    /* return from block */\n");
    fprintf(out, "%d:\n", label);
}

void emit_end(FILE * out) {
    fprintf(out, "    ret\n");
    fprintf(out, ".section        .note.GNU-stack,\"\",@progbits\n");
}

