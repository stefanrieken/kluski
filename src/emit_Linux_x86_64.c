#include <stdio.h>

//#include "kluski.h"
#include "parsta.h"

char * cmdnames[] = { "add", "sub", "and", "or", "xor", "not", "mul", "div", "remainder", "equals", "lt", "gt", "lte", "gte", "lnot", "land", "lor", "dollar"};
char * regnames[] = { "%rax", "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"} ; // NOTE: first reg in this list is for function pointer (= same as return value reg)
char * retnames[] = { "%r12", "%r13", "%r14", "%r15" } ; // return value appears in %rax; then we copy them to one of these. NOTE: r10 and r11 and up should be caller saved!

int num_regnames = sizeof(regnames) / sizeof(char *);
int num_retnames = sizeof(retnames) / sizeof(char *);

void emit_start(FILE * out) {
//    printf(".text\n");
    for (int i=0; i<5;i++) { // The referenced versions of direct-operator functions
        fprintf(out, "%s:\n", cmdnames[i]);
        fprintf(out, "    mov %s, %%rax     /* move to return reg             */\n", regnames[1]);
        fprintf(out, "    %s %s, %%rax\n", cmdnames[i], regnames[2]);
        fprintf(out, "    ret\n");
    }
    fprintf(out, "not:\n");
    fprintf(out, "    mov %s, %s     /* move to return reg             */\n", regnames[1], regnames[0]);
    fprintf(out, "    not %s        /* direct multiply (rax is implicit) */\n", regnames[0]);
    fprintf(out, "    ret\n");
    fprintf(out, "mul:\n");
    fprintf(out, "    mov %s, %%rax     /* move to return reg             */\n", regnames[1]);
    fprintf(out, "    mul %s        /* direct multiply (rax is implicit) */\n", regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, "div:\n");
    fprintf(out, "    mov %s, %%rax      /* dividend in return reg         */\n", regnames[1]);
    fprintf(out, "    div %s            /* rax = result; rdx = rest       */\n", regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, "remainder:\n");
    fprintf(out, "    mov %s, %%rax      /* dividend in return reg         */\n", regnames[1]);
    fprintf(out, "    div %s            /* rax = result; rdx = rest       */\n", regnames[2]);
    fprintf(out, "    mov %%rdx, %%rax      /* return remainder               */\n");
    fprintf(out, "    ret\n");
    fprintf(out, "equals:\n");
    fprintf(out, "    mov $0, %s         /* assume false */ \n", regnames[0]);
    fprintf(out, "    cmp %s, %s\n", regnames[1], regnames[2]);
    fprintf(out, "    jne 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "lt:\n");
    fprintf(out, "    mov $0, %s         /* assume false */ \n", regnames[0]);
    fprintf(out, "    cmp %s, %s\n", regnames[2], regnames[1]);
    fprintf(out, "    jnb 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "gt:\n");
    fprintf(out, "    mov $0, %s         /* assume false */ \n", regnames[0]);
    fprintf(out, "    cmp %s, %s\n", regnames[2], regnames[1]);
    fprintf(out, "    jna 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "lte:\n");
    fprintf(out, "    mov $0, %s         /* assume false */ \n", regnames[0]);
    fprintf(out, "    cmp %s, %s\n", regnames[2], regnames[1]);
    fprintf(out, "    jnle 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "gte:\n");
    fprintf(out, "    mov $0, %s         /* assume false */ \n", regnames[0]);
    fprintf(out, "    cmp %s, %s\n", regnames[2], regnames[1]);
    fprintf(out, "    jnge 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "lnot:\n");
    fprintf(out, "    mov $0, %s\n", regnames[0]);
    fprintf(out, "    cmp $0, %s\n", regnames[1]);
    fprintf(out, "    jne 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "land:\n");
    fprintf(out, "    mov $0, %s    /* assume false */ \n", regnames[0]);
    fprintf(out, "    cmp $0, %s\n", regnames[1]);
    fprintf(out, "    je 0f\n");
    fprintf(out, "    cmp $0, %s\n", regnames[2]);
    fprintf(out, "    je 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "lor:\n");
    fprintf(out, "    mov $0, %s    /* assume false */ \n", regnames[0]);
    fprintf(out, "    cmp $0, %s\n", regnames[1]);
    fprintf(out, "    je 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "    cmp $0, %s\n", regnames[2]);
    fprintf(out, "    je 0f\n");
    fprintf(out, "    mov $1, %s\n", regnames[0]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "if:\n");
    fprintf(out, "    mov $0, %s    /* assume false */ \n", regnames[0]);
    fprintf(out, "    cmp $0, %s\n", regnames[1]);
    fprintf(out, "    jz 0f\n");
    fprintf(out, "    jmp *%s           /* let target return to caller    */\n", regnames[2]);
    fprintf(out, "    ret\n");
    fprintf(out, "0:\n");
    fprintf(out, "    cmp $0, %s        /* have else block?              */\n", regnames[3]);
    fprintf(out, "    jz 0f\n");
    fprintf(out, "    jmp *%s           /* let target return to caller    */\n", regnames[3]);
    fprintf(out, "0:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "loop:\n");
    fprintf(out, "    push %s         /* save block arg to stack */\n", regnames[1]);
    fprintf(out, "0:\n");
    fprintf(out, "    call *0(%%rsp)\n");
    fprintf(out, "    cmp $0, %s\n", regnames[0]);
    fprintf(out, "    jnz 0b\n");
    fprintf(out, "    add $8, %%rsp     /* remove block arg from stack */\n");
    fprintf(out, "    ret\n");
    fprintf(out, "funcall:                /* (demo) function ptr support    */\n");
    for (int i=1; i<num_regnames; i++) {
        fprintf(out, "    mov %s, %s\n", regnames[i], regnames[i-1]);
    }
    fprintf(out, "    jmp *%s           /* let target return to caller    */\n", regnames[0]);
//    fprintf(out, "define:\n"); // should be equivalent to the one in primitives.c, so may as well keep portable
//    fprintf(out, "    mov top_variables(%%rip), %%rax\n");
//    fprintf(out, "    mov %s, 0(%%rax) /* store name                     */\n", regnames[1]);
//    fprintf(out, "    mov %s, 8(%%rax)  /* store value                    */\n", regnames[2]);
//    fprintf(out, "    add $16, %%rax        /* increment top_variables        */\n");
//    fprintf(out, "    mov %%rax, top_variables(%%rip) /* and save */\n");
//    fprintf(out, "    ret\n");
    fprintf(out, "args:\n");
    fprintf(out, "    mov top_variables(%%rip), %%rax\n");
    for (int i=1;i<=num_retnames; i++) {
        fprintf(out, "    cmp $0, %s        /* have arg %d?                    */ \n", regnames[i], i);
        fprintf(out, "    jz %df               /* else done                      */\n", i == 1 ? 1 : 0);
        fprintf(out, "    mov %s,  0(%%rax)  /* store name %d                  */\n", regnames[i], i);
        fprintf(out, "    mov %d(%%rsp), %s   /* find value %d on stack pointer */\n", 8*i, regnames[i], i);
        fprintf(out, "    mov %s, 8(%%rax)   /* store value %d */\n", regnames[i], i);
        fprintf(out, "    add $16, %%rax       /* top_variables++                */\n");
    }
    fprintf(out, "0:\n");
    fprintf(out, "    mov %%rax, top_variables(%%rip) /* and save */\n");
    fprintf(out, "1:\n");
    fprintf(out, "    ret\n");
    fprintf(out, "return:\n");
    fprintf(out, "    mov %s, %s\n", regnames[1], regnames[0]);
    fprintf(out, "    ret\n");
    fprintf(out, ".globl main\n");
    fprintf(out, "main:\n");
    fprintf(out, "    call init\n");
}

int unique_string_idx(ParseStackEntry * entry) {
    int idx = 0;
    StringEntry * e = unique_strings;
    while(e != NULL) { if (e->str == entry->value.str) break;  e = e->next; idx++; }
    return idx;
}

int emit_entry(FILE * out, ParseStack * stack, int from, int n_arg, int n_args, int * stashbase) {
    ParseStackEntry * entry = &(stack->entries[from]);
    switch(entry->type) {
        case PT_INT:
            fprintf(out, "    mov $%d, %s\n", entry->value.num, regnames[n_arg]);
            break;
        case PT_STR:
            fprintf(out, "    lea str%d(%%rip), %s\n", unique_string_idx(entry), regnames[n_arg]);
            break;
        case PT_FUN:
            switch(entry->value.num) {
                case 0: // '+'; all these functions have their own operator
                case 1: // '-'
                case 2: // '&'
                case 3: // '|'
                case 4: // '^'
                    if (n_arg == 0) { // that's the function position; in any other position, function == common argument
                        fprintf(out, "    mov %s, %%rax      /* move to return reg             */\n", regnames[1]);
		        for (int i=2; i<n_args; i++) {
		            fprintf(out, "    %s %s, %%rax      /* direct %s                     */\n", cmdnames[entry->value.num], regnames[i], cmdnames[entry->value.num]);
		        }
                    } else {
                        fprintf(out, "    lea %s(%%rip), %s\n", cmdnames[entry->value.num], regnames[n_arg]);
                    }
                    break;
                case 5: // '~'
                case 6: // '*'; apart from the function call we presently make, this operator requires a different expression form from '+' etc. on x86
                case 7: // '/'
                case 8: // '%'
                case 9: // '='
                case 10: // '<'
                case 11: // '>'
                case 12: // '<='
                case 13: // '>='
                case 14: // '!'
                case 15: // '&&'
                case 16: // '||'
                case 17: // '$'
                    if (n_args < num_regnames) fprintf(out, "    mov $0, %s /* mark end of potential varargs */\n", regnames[n_args]);
                    fprintf(out, "    lea %s(%%rip), %s\n", cmdnames[entry->value.num], regnames[n_arg]); // That's for function pointers
                    if (n_arg == 0) { // that's the function position; in any other position, function == common argument
                        fprintf(out, "    call *%s\n", regnames[0]);
                    }
                    break;
                default: // "funcall", "printnum", "print", ...
                    if (n_args < num_regnames) fprintf(out, "    mov $0, %s /* mark end of potential varargs */\n", regnames[n_args]);
                    fprintf(out, "    lea %s(%%rip), %s\n", primitive_names[entry->value.num], regnames[n_arg]); // That's for function pointers
                    if (n_arg == 0) { // that's the function position; in any other position, function == common argument
                        fprintf(out, "    call *%s\n", regnames[n_arg]);
                    }
                    break;
            }
            break;
//        case PT_REF:
//            break;
        case PT_OPN:
            if (entry->value.num == '(') {
                if (*stashbase != stashptr) // Last subexpr was not stashed
                    fprintf(out, "    mov %s, %s      /* unstash return value           */\n", retnames[(*stashbase)++], regnames[n_arg]);

                // As we generate subexprs in written order to facilitate stash / unstash,
                // we cannot immediately invoke any function subexpr as we meet it; leave this with our caller
                //if (n_arg == 0) { // expression at function position!
                //    fprintf(out, "    call *%s\n", regnames[0]);
                //}

                // Skip sub-expression since it was already written
                return skip_until_close(stack, from)-1;
            } else {
                block_depth++;

                fprintf(out, "    lea 0f(%%rip), %s /* load start of block as arg      */\n", regnames[n_arg]);
                fprintf(out, "    jmp %df             /* jump over the block             */\n", block_depth);
                fprintf(out, "0:                     /* start of block                  */\n");
                fprintf(out, "    mov top_variables(%%rip), %%rax\n");
                fprintf(out, "    push %%rax\n");

                int n_args2 = num_args(stack, from+1); // = -1 if no 'args'
                if (n_args2 != -1) {
                    for (int i=n_args2-1;i>=0;i--) {
                        fprintf(out, "    push %s     /* store arg %d for 'args'       */\n", regnames[i+1], i+1);
                    }
                    if (n_args2 < num_regnames-1) {
                        fprintf(out, "    mov $0, %s     /* mark end of varargs to 'args' */\n", regnames[n_args2+1]);
                    }
                }

                from = emit_code(out, stack, from+1, '}')-1;

                if (n_args2 > 0) {
                    fprintf(out, "    add $%d, %%rsp     /* remove args from stack */\n", n_args2*8);
                }
                fprintf(out, "    pop %s\n", regnames[1]);
                fprintf(out, "    mov %s, top_variables(%%rip)\n", regnames[1]);
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
    fprintf(out, "    mov %%rax, %s      /* stash return value            */\n", retnames[stashptr++]);
}

void emit_move_retval(FILE * out, int n_arg) {
    if (n_arg == 0) fprintf(out, "                        /* retval in %%rax == func ptr reg */\n");
    else fprintf(out, "    mov %s, %s      /* transfer return value to arg   */\n", regnames[0], regnames[n_arg]);
}

void emit_call_subexpr(FILE * out) {
    fprintf(out, "    call *%s          /* call subexpr function          */ \n", regnames[0]);
}

void emit_end(FILE * out) {
    fprintf(out, "    ret\n");
    fprintf(out, ".section        .note.GNU-stack,\"\",@progbits\n");
}

