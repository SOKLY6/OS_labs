all: lab3

lab3: main.c tim_sort.c
	gcc -std=c11 -O2 -Wall -Wextra -Wpedantic main.c tim_sort.c -o lab3 -pthread

strace_lab1: lab3
	strace -f -o lab1.log ./lab3 1000000 -t 4

strace_lab_current: lab3
	strace -f -yy -s 128 \
		-e trace=execve,clone,clone3,futex,mmap,munmap,brk,gettid,clock_gettime,write,exit,exit_group \
		-o lab3.log ./lab3 1000000 -t 4

clean:
	rm -f lab3 lab3.log