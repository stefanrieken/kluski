#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "parsta.h"

int printnum(int a) {
    printf("%d\n", a);
    return a;
}

int print(char * a) {
    printf("%s", a);
    return 0;
}

// Var support
typedef struct Variable {
    char * name;
   union {
        intptr_t num;
        void * ptr;
    } value;
} Variable;

int num_variables;
Variable * variables;
int top_variables;

void init() {
    num_variables = 256;
    variables = malloc(sizeof(Variable) * num_variables);
    top_variables = 0;
}

Variable * slot(char * name) {
    for (int i=top_variables-1; i >=0; i--) {
        Variable * var = &variables[i];
        if (var->name == name) { // exact same string pointer
            return var;
        }
    }
    printf("Runtime error: var %s not found\n", name);
    exit(-1);
//    return NULL;
}

intptr_t define(char * name, intptr_t val) {
    //printf("Defining %s as %d\n", name, val);
    Variable * var = &variables[top_variables++];
    var->name = name;
    var->value.num = val;
    return val;
}

intptr_t get(char * name) {
    Variable * var = slot(name);
    if (var == NULL) return 0;
    return var->value.num;
}

intptr_t set(char * name, intptr_t val) {
    //printf("Setting %s to %d\n", name, val);
    Variable * var = slot(name);
    if (var == NULL) return 0;
    var->value.num = val;
    return val;
}
