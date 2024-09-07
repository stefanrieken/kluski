#include <stdio.h>

#include "parsta.h"

char * cmdnames[] = { "add", "sub", "mul", "div", "remainder", "equals"};
char * regnames[] = { "x8", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"} ; // NOTE: first reg in this list is for function pointer (if needed)
char * retnames[] = { "x9", "x10", "x11", "x12", "x13", "x14", "x15" } ; // return value appears in x0

int num_regnames = sizeof(regnames) / sizeof(char *);
int num_retnames = sizeof(retnames) / sizeof(char *);

void emit_start(FILE * out) {
    // Improvement suggestions:
    // 1) only emit asm utility functions that are actually referenced (e.g. at end instead of start)
    // 2) clang: define built-in-support functions without underscore, so as not to interfere with C primitives
    fprintf(out, ".align 4\n");
    fprintf(out, "add:\n");
    fprintf(out, "    add x0, %s, %s      /* direct add into return reg     */\n", regnames[1], regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "sub:\n");
    fprintf(out, "    sub x0, %s, %s      /* direct add into return reg     */\n", regnames[1], regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "mul:\n");
    fprintf(out, "    mul x0, %s, %s      /* direct mul into return reg     */\n", regnames[1], regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "div:\n");
    fprintf(out, "    div x0, %s, %s      /* direct mul into return reg     */\n", regnames[1], regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "remainder:             /* e.g. 13 %% 4 (no arm native %%)  */\n");
    fprintf(out, "    udiv %s, %s, %s     /* tmp = 13 / 4                   */\n", regnames[3], regnames[1], regnames[2]);
    fprintf(out, "    msub %s, %s, %s, %s /* result = 13 - (tmp * 4)        */\n", regnames[1], regnames[3], regnames[2], regnames[1]);
    fprintf(out, "    ret\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "equals:\n");
    fprintf(out, "    ret\n"); // TODO
    fprintf(out, ".align 4\n");
    fprintf(out, "not:\n");
    fprintf(out, "    cmp $0, %s\n", regnames[1]);
    fprintf(out, "    je 0f\n");
    fprintf(out, "    mov $0, %s\n", regnames[0]);
    fprintf(out, "    ret\n");
    fprintf(out, "0:\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "    ret\n");
    fprintf(out, "_if:\n");
    fprintf(out, "    cmp %s, #0\n", regnames[1]);
    fprintf(out, "    beq 0f\n");
    fprintf(out, "    br %s           /* let target return to caller    */\n", regnames[2]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "_funcall:               /* (demo) function ptr support    */\n"); // 'funcal' _is_ a 'C primitive'
    for (int i=1; i<num_regnames; i++) {
        fprintf(out, "    mov %s, %s\n", regnames[i-1], regnames[i]);
    }
    fprintf(out, "    br %s               /* let target return to caller     */\n", regnames[0]);
    fprintf(out, ".align 4\n");
    fprintf(out, "_args:\n");
    fprintf(out, "    adrp x7, _top_variables@PAGE\n"); // TODO hijacking an arg register here.
    fprintf(out, "    add x7, x7, _top_variables@PAGEOFF\n"); // TODO hijacking an arg register here.
    fprintf(out, "    ldr x8, [x7]\n");
    for (int i=1;i<=num_retnames; i++) {
        fprintf(out, "    cmp %s, #0        /* have arg %d?                    */ \n", regnames[i], i);
        fprintf(out, "    beq %df               /* else done                      */\n", i == 1 ? 1 : 0);
        fprintf(out, "    str %s, [x8, #0]  /* store name %d                  */\n", regnames[i], i);
        fprintf(out, "    ldr %s, [sp, #%d]   /* find value %d on stack pointer */\n", regnames[i], 16*(i-1), i);
        fprintf(out, "    str %s, [x8, #8]   /* store value %d */\n", regnames[i], i);
        fprintf(out, "    add x8, x8, #16       /* top_variables++                */\n");
    }
    fprintf(out, "0:\n");
    fprintf(out, "    str x8, [x7] /* and save */\n");
    fprintf(out, "1:\n");
    fprintf(out, "    ret\n");
    fprintf(out, ".globl _main\n");
    fprintf(out, ".align 4\n");
    fprintf(out, "_main:\n");
    fprintf(out, "    stp fp, lr, [sp, #-0x10]!       /* save fp, lr for bl */\n");
    fprintf(out, "    bl _init\n");
}

int emit_entry(FILE * out, ParseStack * stack, int from, int n_arg, int n_args, int * stashbase) {
    ParseStackEntry * entry = &(stack->entries[from]);
    int idx;
    switch(entry->type) {
        case PT_INT:
            fprintf(out, "    mov %s, %d\n", regnames[n_arg], entry->value.num);
            break;
        case PT_STR:
            idx = 0;
            StringEntry * e = unique_strings;
            while(e != NULL) { if (e->str == entry->value.str) break;  e = e->next; idx++; }
            fprintf(out, "    adr %s, str%d\n", regnames[n_arg], idx);
            break;
        case PT_FUN:
            switch(entry->value.num) {
                case 0: // '+'
                case 1: // '*'
                    if (n_arg == 0) {
                        // if function is invoked directly, use processor's math operations
                        for (int i=2; i<n_args; i++) {
                            fprintf (out, "    %s x0, x0, %s      /* %s argument # %d               */ \n", cmdnames[entry->value.num], regnames[i], cmdnames[entry->value.num], i);
                        }
                    } else {
                        // if this is a function pointer argument, point to a real function (which by the way takes at most two args)
                        fprintf (out, "    adr %s, %s\n", regnames[n_arg], cmdnames[entry->value.num]);
                    }
                    break;
                case 2: // '%' -- support function's written form differs from primitive name; solve by also using 'cmdname' for this
                    if (n_arg == 0) {
                        // if function is invoked directly, call it
                        fprintf (out, "    bl %s            /* call '%s'          */\n", cmdnames[entry->value.num], primitive_names[entry->value.num]);
                        fprintf(out, "    ldp fp, lr, [sp], #0x10    /* restore fp, lr after bl */\n");
                    } else {
                        // if this is a function pointer argument, just supply the pointer
                        fprintf (out, "    adr %s, %s\n", regnames[n_arg], cmdnames[entry->value.num]);
                    }
                    break;
                default: // "funcall", "printnum", "print", ...
                    if (n_arg == 0) { // that's the function position; in any other position, function == common argument
                        fprintf (out, "    bl _%s\n", primitive_names[entry->value.num]);
                    } else {
                        // Apparently we're only pointing to the function
                        fprintf (out, "    adr %s, _%s\n", regnames[n_arg], primitive_names[entry->value.num]); // That's for function pointers
                    }
                    break;
            }
            break;
//        case PT_REF:
//            break;
        case PT_OPN:
            if (entry->value.num == '(') {
                if (*stashbase != stashptr) // Last subexpr was not stashed
                    fprintf(out, "    mov %s, %s          /* unstash return value           */\n", regnames[n_arg], retnames[(*stashbase)++]);
                // As we generate subexprs in written order to facilitate stash / unstash,
                // we cannot immediately invoke any function subexpr as we meet it; leave this with our caller
                //if (n_arg == 0) { // expression at function position!
                //    fprintf(out, "    bl %s%s\n", regnames[0]);
                //}

                // Skip sub-expression since it was already written
                return skip_until_close(stack, from)-1;
            } else {
                block_depth++;

                fprintf(out, "    adr %s, 0f          /* load start of block as arg     */\n", regnames[n_arg]);
                fprintf(out, "    b %df                /* jump over the block            */\n", block_depth);
                fprintf(out, ".align 4\n");
                fprintf(out, "0:                      /* start of block                 */\n");
                fprintf(out, "    stp fp, lr, [sp, #-0x10]!       /* save fp, lr for bl */\n");

                int n_args2 = num_args(stack, from+1); // = -1 if no 'args'
                if (n_args2 != -1) {
                    for (int i=n_args2-1;i>=0;i--) {
                        fprintf(out, "    str %s, [sp, #-16]!     /* store arg %d for 'args'       */\n", regnames[i+1], i+1);
                    }
                    if (n_args2 < num_regnames-1) {
                        fprintf(out, "    mov %s, #0     /* mark end of varargs to 'args' */\n", regnames[n_args2+1]);
                    }
                }

                from = emit_code(out, stack, from+1, '}')-1;

                if (n_args2 > 0) {
                    fprintf(out, "    add sp, sp, #%d     /* remove args from stack */\n", n_args2*16);
                }
                fprintf(out, "    ldp fp, lr, [sp], #0x10    /* restore fp, lr after bl */\n");
                fprintf(out, "    ret                 /* return from block              */\n");
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
    fprintf(out, "    mov %s, x0          /* stash return value             */\n", retnames[stashptr++]);
}

void emit_move_retval(FILE * out, int n_arg) {
    if (n_arg == 1) fprintf(out, "                        /* retval reg x0 == arg reg x0    */\n");
    else fprintf(out, "    mov %s, x0          /* transfer return value to arg   */\n", regnames[n_arg]);
}

void emit_call_subexpr(FILE * out) {
    fprintf(out, "    blr %s          /* call subexpr function          */ \n", regnames[0]);
}

void emit_end(FILE * out) {
    fprintf(out, "    ldp fp, lr, [sp], #0x10    /* restore fp, lr after bl */\n");
    fprintf(out, "    ret\n");
}
