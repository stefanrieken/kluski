#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "parsta.h"

// Return position AFTER close
int skip_until_close(ParseStack * stack, int from) {
    int n_open = 0;
    char closing_char = stack->entries[from].value.num == '{' ? '}' : ')';
    int i;
    for (i=from; i<stack->length; i++) {
        ParseStackEntry * entry = &(stack->entries[i]);
        if (entry->type == PT_OPN) {
            n_open++;
        }
        else if(entry->type == PT_CLS && entry->value.num != ';') {
            n_open--;
            if(n_open == 0) {
                if (entry->value.num != closing_char) printf("Bracket mismatch2\n");
                return i+1;
            }
        }
    }
    printf("Error: no closing bracket found\n");
    return i;
}

// Return position AFTER close IF close is ';'
// So at any rate, return position AFTER expression
int emit_subexprs(FILE * out, ParseStack * stack, int from) {
    int i = from;
    int n_arg = 0;

    // Optimization: don't temporarily save retval for last subexpr;
    // instead immediately move it into target register
    int to_stash = -1;

    for (; i<stack->length; i++) {
 
        ParseStackEntry * entry = &stack->entries[i];
        if(entry->type == PT_CLS) {
//            printf("close in subexprs: %d %c\n", i, entry->value.num);
            if(entry->value.num == ';') { i += 1; }
            break;
        }

        if (entry->type == PT_OPN) {
            if (entry->value.num == '(') {
               if (to_stash > -1) { emit_save_retval(out); } // for previous subexpr
               i = emit_code(out, stack, i+1, ')')-1; // correcting for upcoming i++
               to_stash = n_arg;
            } else {
                i = skip_until_close(stack, i)-1; // correcting for upcoming i++
            }
        }
        n_arg++;
    }
    if (to_stash > -1) emit_move_retval(out, to_stash); // for previous subexpr

    return i;
}

// After subexprs, emit this expr;
// Return position AFTER close IF close is ';'
// So at any rate, return position AFTER expression
int emit_this_expr(FILE * out, ParseStack * stack, int from, int stashbase) {
    int n_arg = 0;

    int i = from;
    if (stack->entries[i].type != PT_OPN) {
        // Emit function subexpr in written order so that stash / unstash succeeds;
        // but wait until end to actually trigger eval.
        //
        // Skip other function statements in full until at postix position.
        // TODO: distinguish subexpr from block
        i++;
        n_arg++;
    }

    for (; i<stack->length; i++) {
        ParseStackEntry * entry = &(stack->entries[i]);
        if(entry->type == PT_CLS) {
//            printf("close in this: %d %c\n", i, entry->value.num);
            if (entry->value.num == ';') { i += 1; }
            break;
        }
        i = emit_entry(out, stack, i, n_arg++, 0, &stashbase);
    }
    // And emit function
    if (stack->entries[from].type != PT_OPN) {
        emit_entry(out, stack, from, 0, n_arg, &stashbase);
    } else {
        emit_call_subexpr(out);
    }

    return i;
}

int stashptr;

// Return position AFTER emitted code, including close
int emit_code(FILE * out, ParseStack * stack, int from, char until) {
    // We may be inside a sub-expression; so stashptr need not be zero
    int saved_stashptr = stashptr;

    while(from < stack->length) {
        // Now emitting a single expression, with possible sub-expressions

        int from2 = emit_subexprs(out, stack, from);
        int from3 = emit_this_expr(out, stack, from, saved_stashptr);
        // sanity check: do both functions agree on end of current expression / start of new?
        if (from2 != from3) printf("Error emitting code: %d %d\n", from2, from3);
        //else printf("Nice: %d %d\n", from2, from3);
        from = from3;
        // Reset for next expression if this is a sequence (a block; in wich case expect zero)
        stashptr = saved_stashptr;

        if (from < stack->length) {
            ParseStackEntry * entry = &stack->entries[from];
            if (entry->type == PT_CLS && entry->value.num == until) return from+1;
            if (entry->type == PT_CLS && entry->value.num != until) { printf ("Erreur\n");};
        }
    }
    return from;
}

void emit_strings(FILE * out) {
//fprintf(out, ".section .rodata\n");
    StringEntry * entry = unique_strings;
    int i = 0;
    while(entry != NULL) {
        fprintf(out, "str%d:    .ascii \"%s\\0\"\n", i++, entry->str);
        entry = entry->next;
    }
//fprintf(out, ".section .text\n");
}

int block_depth;

#define PRIM_ARGS 26
// Utility for writing code for 'args'
int num_args(ParseStack * stack, int from) {
    int n_args=0;
    if (stack->entries[from].type == PT_FUN && stack->entries[from].value.num == PRIM_ARGS) {
        for (int i=from+1; i<stack->size;i++) {
            if (stack->entries[i].type == PT_CLS) break;
            n_args++;
        }
        return n_args;
    }
    // else
    return -1; // no 'args' defined
}
