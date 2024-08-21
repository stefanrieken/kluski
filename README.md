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
The only current backend generates x86-64 GNU assembly code, which can then be
compiled further.

Typing `make` should build the `kluski` executable, _and_ let it compile
`test.kluski` into `test.s`, _and_ compile that into the `test` executable.

Next, you can test the return value of the `test` executable:

        ./test; echo $?

If your system is not x86-64, compilation of `test.s` will likely fail; but you
should at least be able to admire the generated assembly -- even though
admittedly even the best x86 assembly code reads like poetry written in poo.
(More backends to follow.)

You may notice that we generate direct instructions ('+') and calls to both
primitive assembly functions ('*') and primitive C functions ('?' = print).
