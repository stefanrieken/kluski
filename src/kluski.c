#include <stdlib.h>
#include <stdio.h>

#include "kluski.h"

/**
 * Generate machine code for prefixed calculation expressions with
 * bracketed sub-expressions, assigning registers according to the
 * applicable C calling convention.
 *
 * Use a compile-time stack (CTS) to keep track of what arguments
 * have already been parsed but are yet to be assigned a register,
 * which can only reliably be done at close of a (sub)expression.
 *
 * All this may be read as a prelude to compile Pasta code, which
 * additionally supports strings, general variables and (inlined)
 * code blocks.
 */

// Emit topmost expression as code
void emit_expression(CtStack * cts, int nth_subexpr) {
    // For now, just pump out pseudo code on screen

    // Find opening bracket
    if (cts->values[cts->length-1].command == CTS_OPEN) return; // empty statement
    int start = cts->length-1;
    for (; start>= 0; start--) {
        if(cts->values[start].command == CTS_OPEN) break;
    }

    // Emit that sh
    for (int i=start+2; i<cts->length;i++) {
        emit_code(&cts->values[i], i-(start+1));
    }
    if(start+1 < cts->length) { // failsafe against empty expression
        emit_code(&cts->values[start+1], 0);
    }

    // Reduce CTS
    cts->length = start;
    
//    printf ("pushing retval\n");
    cts_push(cts, CTS_RETV, nth_subexpr);
}

void parse (CtStack * cts) {
    int ch = getchar();

    // Outermost bracket is implicit (and ended by newline);
    // still, register an 'open' for consistency of handling
    emit_start();
    cts_push(cts, CTS_OPEN, ch);

    int n_subexprs = 0;

    while (ch != EOF) {
        while (ch == '#') { do { ch = getchar(); } while (ch != '\n'); ch = getchar(); } // comments
        if (ch == ' ') { ch = getchar(); continue; } // whitespace
        if(cts->values[cts->length-1].command == CTS_RETV) emit_save_retval(cts->values[cts->length-1].value);
        if (ch >= '0' && ch <= '9') { // number
           int numval = ch - '0';
           ch = getchar();
           while (ch>= '0' && ch <= '9') {
               numval = numval * 10 + (ch - '0');
               ch = getchar();
           }
           cts_push(cts, CTS_ARGV, numval);
        } else {
            switch(ch) {
                case '+':
                case '*':
                case '?':
                    cts_push(cts, CTS_FUNC, ch);
                    break;
                case '(':
                    cts_push(cts, CTS_OPEN, 0);
                    break;
                case '\n':
//                    for (int i=0; i<cts->length;i++) printf("%s ", cmdnames[cts->values[i].command]);
//                    printf("\n");
                      n_subexprs = 0;
                    // and fall through
                case ')':
                    emit_expression(cts, n_subexprs++);
                    if (ch == '\n') cts_push(cts, CTS_OPEN, 0); // start new implicit bracket
                    break;
                default:
                    printf("Unrecognized input.\n");
                    break;
            }
            ch = getchar();
        }
    }
    
    emit_end();
}

int main (int argc, char ** argv) {
    CtStack cts = { 256, 0, malloc(sizeof(CtsEntry) * 256) };

    parse(&cts);
}
