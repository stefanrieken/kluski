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
void emit_expression(FILE * out, CtStack * cts, int nth_subexpr) {
    // For now, just pump out pseudo code on screen

    // Find opening bracket
    if (cts->values[cts->length-1].command == CTS_OPEN) return; // empty statement
    int start = cts->length-1;
    for (; start>= 0; start--) {
        if(cts->values[start].command == CTS_OPEN) break;
    }

    // Emit that sh
    for (int i=start+2; i<cts->length;i++) {
        emit_code(out, &cts->values[i], i-(start+1), cts->length - (start+1));
    }
    if(start+1 < cts->length) { // failsafe against empty expression
        emit_code(out, &cts->values[start+1], 0, cts->length - (start+1));
    }

    // Reduce CTS
    cts->length = start;

//    printf ("pushing retval\n");
    cts_push(cts, CTS_RETV, nth_subexpr);
}

void parse (CtStack * cts, FILE * in, FILE * out) {
    int ch = fgetc(in);

    // Outermost bracket is implicit (and ended by newline);
    // still, register an 'open' for consistency of handling
    emit_start(out);
    cts_push(cts, CTS_OPEN, ch);

    int n_subexprs = 0;

    while (ch != EOF) {
        while (ch == '#') { do { ch = fgetc(in); } while (ch != '\n'); ch = fgetc(in); } // comments
        if (ch == ' ') { ch = fgetc(in); continue; } // whitespace
        if(cts->values[cts->length-1].command == CTS_RETV) emit_save_retval(out, cts->values[cts->length-1].value);
        if (ch >= '0' && ch <= '9') { // number
           int numval = ch - '0';
           ch = fgetc(in);
           while (ch>= '0' && ch <= '9') {
               numval = numval * 10 + (ch - '0');
               ch = fgetc(in);
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
                    emit_expression(out, cts, n_subexprs++);
                    if (ch == '\n') cts_push(cts, CTS_OPEN, 0); // start new implicit bracket
                    break;
                default:
                    printf("Unrecognized input.\n");
                    break;
            }
            ch = fgetc(in);
        }
    }
    
    emit_end(out);
}

int main (int argc, char ** argv) {
    CtStack cts = { 256, 0, malloc(sizeof(CtsEntry) * 256) };

    FILE * infile = stdin;
    FILE * outfile = stdout;
    if (argc > 1) { infile  = fopen(argv[1], "r"); }
    if (argc > 2) { outfile = fopen(argv[2], "w"); }

    parse(&cts, infile, outfile);

    if (argc > 1) { fclose(infile ); }
    if (argc > 2) { fclose(outfile); }
}
