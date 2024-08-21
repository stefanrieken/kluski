all: test

kluski: src/kluski.c src/emit_gasm_x64.c
	gcc -Wall -Wunused $^ -o $@

test.s: kluski test.kluski
	./kluski <test.kluski >test.s

test: test.s src/primitives.c
	gcc test.s src/primitives.c -o test

clean:
	rm kluski test
