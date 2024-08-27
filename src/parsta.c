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
    "+", "*", "printnum", "eval", "print"
};
# define num_primitives (sizeof(primitive_names) / sizeof(char *))

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

void parse (FILE * in, FILE * out, ParseStack * stack) {
    int ch = fgetc(in);

    while (ch != EOF) {
        while (ch == '#') { do { ch = fgetc(in); } while (ch != '\n'); ch = fgetc(in); } // comments
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
                if (length % 256 == 0) buffer = realloc(buffer, sizeof(char) * (length + 256));
                ch = fgetc(in);
                if (ch == '\"') ch = '\0'; // switch terminator values
                buffer[length++] = ch;
            } while (ch != '\0');
            push_str(stack, PT_STR, unique_string(buffer));
            ch = fgetc(in);
        } else {
            // Parse label; very comparable to above, but not quit the same.
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
            default:
                // Not all defined types have implementations yet; that's ok
                break;
        }
    }
    return stack->length;
}

// Return position AFTER close
int skip_until_close(ParseStack * stack, int from) {
    int n_open = 0;
    char closing_char = stack->entries[from].value.num == '{' ? '}' : ')';
//printf("Saw '%c' so want '%c' (type is %d)\n", stack->entries[from].value.num, closing_char, stack->entries[from].type);
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
    int n_arg = 0;
    int i = from;
    for (; i<stack->length; i++) {
        ParseStackEntry * entry = &stack->entries[i];
        if(entry->type == PT_CLS) {
//            printf("close in subexprs: %d %c\n", i, entry->value.num);
            if(entry->value.num == ';') { i += 1; }
            break;
        }

        if (entry->type == PT_OPN) {
            if (entry->value.num == '(') {
                i = emit_code(out, stack, i+1, ')')-1; // correcting for upcoming i++
                //printf("    stash result\n");
                emit_save_retval(out);
            } else {
                i = skip_until_close(stack, i)-1; // correcting for upcoming i++
            }
        }
        n_arg++;
    }
    return i;
}

// After subexprs, emit this expr;
// Return position AFTER close IF close is ';'
// So at any rate, return position AFTER expression
int emit_this_expr(FILE * out, ParseStack * stack, int from, int stashbase) {
    int n_arg = 1;

    // skip subexpression or block at function position
    int i = from;
    if (stack->entries[i].type == PT_OPN) {
        i = skip_until_close(stack, i)-1;
    }

    i++; // go past first arg or end of subexpression

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
    emit_entry(out, stack, from, 0, n_arg, &stashbase);

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
fprintf(out, ".section .rodata\n");
    StringEntry * entry = unique_strings;
    int i = 0;
    while(entry != NULL) {
        fprintf(out, "str%d:    .ascii \"%s\\0\"\n", i++, entry->str);
        entry = entry->next;
    }
fprintf(out, ".section .text\n");
}

int main (int argc, char ** argv) {
    ParseStack stack = { 256, 0, malloc(sizeof(ParseStackEntry) * 256) };
    unique_strings = NULL;

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
