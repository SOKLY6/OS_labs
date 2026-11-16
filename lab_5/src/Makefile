.PHONY: all clean strace_lab1_pr2 strace_lab_current2 strace_lab1_pr1 strace_lab_current1

all: lib1.so lib2.so program1 program2

lib1.so: lib1.c functions.h
	gcc -std=c11 -Wall -Wextra -fPIC -shared lib1.c -o lib1.so -lm

lib2.so: lib2.c functions.h
	gcc -std=c11 -Wall -Wextra -fPIC -shared lib2.c -o lib2.so -lm

program1: program1.c functions.h lib1.so
	gcc -std=c11 -Wall -Wextra program1.c -L. -l1 -Wl,-rpath,. -o program1 -lm

program2: program2.c
	gcc -std=c11 -Wall -Wextra program2.c -o program2 -ldl -lm

strace_lab1_pr2: all
	strace -f -o lab1.log ./program2

strace_lab_current2: all
	strace -f -yy -s 128 \
		-e trace=execve,openat,newfstatat,fstat,read,write,close,mmap,mprotect,munmap,exit_group \
		-o lab5.log ./program2

strace_lab1_pr1: all
	strace -f -o lab1_program1.log ./program1

strace_lab_current1: all
	strace -f -yy -s 128 \
		-e trace=execve,openat,newfstatat,fstat,read,write,close,mmap,mprotect,munmap,exit_group \
		-o lab5_program1.log ./program1

clean:
	rm -f lib1.so lib2.so program1 program2