#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "parsta.h"

/**
 * A parse stack (= budget version of a parse tree) for Pasta code,
 * providing the amount of analysis required to emit assembly in a
 * logical order (= sub-expressions first).
 */

char * primitive_names[] = {
    "+", "*", "\%", "if", "funcall", "printnum", "print", "define", "get", "set", "args"
};
int num_primitives = (sizeof(primitive_names) / sizeof(char *));
#define PRIM_DEFINE 7
#define PRIM_GET 8
#define PRIM_ARGS 10

ParseStackEntry * push (ParseStack * stack, ParseStackType type, int value) {
    if (stack->length >= stack->size) { printf ("Parse stack overflow\n"); exit(-1); }
    ParseStackEntry * entry = &stack->entries[stack->length++];
    entry->type = type;
    entry->value.num = value;
    return entry;
}

ParseStackEntry * push_str (ParseStack * stack, ParseStackType type, char * value) {
    ParseStackEntry * entry = push(stack, type, 0);
    entry->value.str = value;
    return entry;
}

StringEntry * unique_strings;

char * unique_string(char * value) {
    StringEntry * entry = unique_strings;
    while (entry != NULL) {
        if (strcmp(entry->str, value) == 0) {
            free(value); // free parsed duplicate
            return entry->str;
        }
        entry = entry->next;
    }
    // it's new alright
    entry = (StringEntry *) malloc(sizeof(StringEntry));
    entry->next = unique_strings;
    entry->str = value;
    unique_strings = entry;
    return entry->str;
}

#define BUF_LEN 256

void parse (FILE * in, FILE * out, ParseStack * stack) {
    int ch = fgetc(in);

    while (ch != EOF) {
        while (ch == '#') { do { ch = fgetc(in); } while (ch != '\n'); continue; } // comments
        if (ch == ' ' || ch == '\t' || ch == '\n') { ch = fgetc(in); continue; } // whitespace
        if (ch >= '0' && ch <= '9') { // number
            int numval = ch - '0';
            ch = fgetc(in);
            while (ch>= '0' && ch <= '9') {
                numval = numval * 10 + (ch - '0');
                ch = fgetc(in);
            }
            push(stack, PT_INT, numval);
        } else if (ch == '{' || ch == '(') {
            push(stack, PT_OPN, ch);
            ch = fgetc(in);
        } else if (ch == '}' || ch == ')' || ch == ';') {
            push(stack, PT_CLS, ch);
            ch = fgetc(in);
        } else if (ch == '\"') {
            char * buffer = NULL;
            int length = 0;
            do  {
                if (length % BUF_LEN == 0) buffer = realloc(buffer, sizeof(char) * (length + BUF_LEN));
                ch = fgetc(in);
                // NOTE: for escape sequences we piggyback on the assembler
                if (ch == '\"') ch = '\0'; // switch terminator values
                buffer[length++] = ch;
            } while (ch != '\0');
            push_str(stack, PT_STR, unique_string(buffer));
            ch = fgetc(in);
        } else {
            // Parse label; very comparable to above, but not quit the same.
            char * buffer = NULL;
            int length = 0;
            do {
                if (length % BUF_LEN == 0) buffer = realloc(buffer, sizeof(char) * (length + BUF_LEN));
                buffer[length++] = ch; ch = fgetc(in);
            } while (ch != EOF && ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' && ch != '(' && ch != ')' && ch != '{' && ch != '}' && ch != ';');
                if (length % BUF_LEN == 0) buffer = realloc(buffer, sizeof(char) * (length + 1));
            buffer[length++] = '\0';
            //printf("Word of the day: *%s*\n", buffer);
            int idx = -1;
            for (int i=0; i<num_primitives;i++) {
                if (strcmp(primitive_names[i], buffer) == 0) {
                    idx = i; break;
                }
            }

            if (idx == -1) {
                // Not recognized as builtin; so assume it's a var
//                push_str(stack, PT_REF, unique_string(buffer));
                push(stack, PT_OPN, '(');
                push(stack, PT_FUN, PRIM_GET);
                push_str(stack, PT_STR, unique_string(buffer));
                push(stack, PT_CLS, ')');
                
            } else {
                push(stack, PT_FUN, idx);
            }
        }
    }
}

// Recurse printing to demo how the parse stack can be read non-flat,
// so that it is possible to count current argument number
int print_expr(ParseStack * stack, int from, char until) {
//    int n_arg = 0;

    for(int i=from; i<stack->length; i++) {
        ParseStackEntry * entry = &stack->entries[i];
        switch(entry->type) {
            case PT_INT:
                printf("%d ", entry->value.num);
//                printf("[%d] ", n_arg++);
                break;
            case PT_STR:
                printf("\"%s\" ", (char *) entry->value.str);
                break;
            case PT_FUN:
                printf("%s ", primitive_names[entry->value.num]);
//                printf("[%d] ", n_arg++);
                break;
//            case PT_REF:
//                printf("%s ", entry->value.str);
//                break;
            case PT_OPN:
                printf("%c ", entry->value.num);
                i = print_expr(stack, i+1, entry->value.num == '{' ? '}' : ')');
                break;
            case PT_CLS:
                printf("%c ", entry->value.num);
                if (entry->value.num == until) return i;
                else if (entry->value.num != ';') printf("Bracket mismatch\n");
//                else if (entry->value.num == ';') n_arg = 0;
                break;
        }
    }
    return stack->length;
}


int main (int argc, char ** argv) {
    ParseStack stack = { 1024, 0, malloc(sizeof(ParseStackEntry) * 1024) };
    unique_strings = NULL;
    block_depth = 0;

    FILE * infile = stdin;
    FILE * outfile = stdout;
    if (argc > 1) { infile  = fopen(argv[1], "r"); }
    if (argc > 2) { outfile = fopen(argv[2], "w"); }

    parse(infile, outfile, &stack);
    print_expr(&stack, 0, EOF);
    printf("\n");
    
    emit_strings(outfile);
    emit_start(outfile);
    emit_code(outfile, &stack, 0, EOF);
    emit_end(outfile);
    printf("\n");

    if (argc > 1) { fclose(infile ); }
    if (argc > 2) { fclose(outfile); }
}
