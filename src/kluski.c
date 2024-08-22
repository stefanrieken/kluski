#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

// This list of known primitives is a little bit overkill, but it
// spares us from parsing duplicate strings.

char * primitive_names[] = {
    "+", "*", "printnum"
};
# define num_primitives (sizeof(primitive_names) / sizeof(char *))

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
        if (ch == ' ' || ch == '\t') { ch = fgetc(in); continue; } // whitespace
        if(cts->values[cts->length-1].command == CTS_RETV) emit_save_retval(out, cts->values[cts->length-1].value);
        if (ch >= '0' && ch <= '9') { // number
           int numval = ch - '0';
           ch = fgetc(in);
           while (ch>= '0' && ch <= '9') {
               numval = numval * 10 + (ch - '0');
               ch = fgetc(in);
           }
           cts_push(cts, CTS_ARGV, numval);
        } else if (ch == '(') {
            cts_push(cts, CTS_OPEN, 0);
            ch = fgetc(in);
        } else if (ch == ')' || ch == '\n' || ch == ';') {
            if (ch == '\n' || ch == ';') n_subexprs = 0;
            emit_expression(out, cts, n_subexprs++);
            if (ch == '\n' || ch == ';') cts_push(cts, CTS_OPEN, 0); // start new implicit bracket
            ch = fgetc(in);
        } else {
            char buffer[256];
            int length = 0;
            do {
                buffer[length++] = ch; ch = fgetc(in);
            } while (ch != EOF && ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' && ch != '(' && ch != ')' && ch != '{' && ch != '}' && ch != ';');
            buffer[length++] = '\0';
            //printf("Word of the day: *%s*\n", buffer);
            int idx = -1;
            for (int i=0; i<num_primitives;i++) {
                if (strcmp(primitive_names[i], buffer) == 0) {
                    idx = i; break;
                }
            }

            if (idx == -1) printf("Unrecognized command: %s\n", buffer);
            else {
                cts_push(cts, CTS_FUNC, idx);
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
