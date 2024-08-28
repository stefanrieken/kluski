# Parsta
Parsta compiles Pasta code by way of a parse stack (= the budget version of a
parse tree).

## Status, and Build
The present compiler recognizes a limitied number of built-in demo commands.
Alongside integers and strings, it can handle blocks and sub-expressions. No
support for variables yet; this mainly requires organization at runtime.

There currently are two backends: x86-64 Linux (gcc) and arm64 MacOS (clang). I
expect e.g. the combination of arm64 and Linux to be slightly different again.
At any rate, the Makefile selects a file called emit_{os}_{arch}.c, and if your
port is missing, you can try adding it.

Otherwise, typing `make` should build the `parsta` executable, _and_ let it
compile `test.pasta` into `test.s`, _and_ compile that into the `test`
executable.

Next, you can run the `test` executable, referring to the input file to learn
what it does:

        ./test

You may notice that we generate both direct math instructions and primitives
written both in assembly (a.o. for math operations as function pointers) and
in C.
