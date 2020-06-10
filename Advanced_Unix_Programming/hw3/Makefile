all: test0 test1 test2 test0s test1s test2s

start.o: start.asm
	yasm -f elf64 start.asm -o start.o

libminic.o: libminic.asm
	yasm -f elf64 libminic.asm -o libminic.o

libminic.so: libminic.o
	ld -shared -o libminic.so libminic.o

test0.o: test0.c
	gcc -c -g -Wall -fno-stack-protector test0.c

test0: test0.o start.o libminic.o
	gcc -nostdlib -o test0 test0.o start.o libminic.o

test0s: test0.o start.o libminic.so
	gcc -nostdlib -o test0s test0.o start.o libminic.so

test1.o: test1.c
	gcc -c -g -Wall -fno-stack-protector test1.c

test1: test1.o start.o libminic.o
	gcc -nostdlib -o test1 test1.o start.o libminic.o

test1s: test1.o start.o libminic.so
	gcc -nostdlib -o test1s test1.o start.o libminic.so

test2.o: test2.c
	gcc -c -g -Wall -fno-stack-protector test2.c

test2: test2.o start.o libminic.o
	gcc -nostdlib -o test2 test2.o start.o libminic.o

test2s: test2.o start.o libminic.so
	gcc -nostdlib -o test2s test2.o start.o libminic.so

clean:
	rm libminic.o libminic.so start.o test0.o test0 test1.o test1 test2.o test2 test0s test1s test2s
