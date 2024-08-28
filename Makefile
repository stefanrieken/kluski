all: test

parsta: src/parsta.c src/emit.c src/emit_$(shell uname -s)_$(shell uname -m).c
	gcc -Wall -Wunused $^ -o $@

test.s: parsta test.pasta
	./parsta test.pasta test.s

test: test.s src/primitives.c
	gcc -Os test.s src/primitives.c -o test

clean:
	rm -f parsta test test.s
