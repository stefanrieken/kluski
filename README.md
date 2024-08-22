# Kluski
Generates machine code for prefixed calculation expressions with bracketed
sub-expressions, assigning registers according to the applicable C calling
convention.

It uses a compile-time stack (CTS) to keep track of what arguments have already
been parsed but are yet to be assigned a register, which can only reliably be
done just before invoking a (sub)expression.

All this may be read as a prelude to compile Pasta code, which additionally
supports strings, general variables and (inlined) code blocks.

## What's in a Name
The goal of Kluski is to become a compiler for the Pasta language.

Being a compiler, the idea of a 'dumpling' seems appropriate; starting off as
a Polish notation calculation processor, a Polish dumpling seems appropriate.
(This is a plain type; we can always upgrade the project to become Pierogi.)

## Status, and Build
There currently ar two backends: x86-64 Linux (gcc) and arm64 MacOS (clang). I
expect e.g. the combination of arm64 and Linux to be slightly different again.
At any rate, the Makefile selects a file called emit_<os>_<arch>.c, and if your
port is missing, you can try adding it.

Otherwise, typing `make` should build the `kluski` executable, _and_ let it
compile `test.kluski` into `test.s`, _and_ compile that into the `test`
executable.

Next, you can run the `test` executable, referring to the input file to know
what it does:

        ./test

You may notice that we generate both direct instructions (in case of '+') as
well as calls to both primitive assembly functions ('*') and primitive C
functions ('?' = print).
