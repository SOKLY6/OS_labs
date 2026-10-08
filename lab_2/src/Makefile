.PHONY: all clean

all: parent child

parent: parent.c
	gcc -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L parent.c -o parent

child: child.c
	gcc -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L child.c -o child

strace_lab1: parent
	strace -f -o lab1.log ./parent

strace_lab_current: parent
	strace -f -yy -s 128 \
  -e trace=execve,clone,pipe2,dup3,openat,read,write,close,wait4,exit_group,rt_sigaction \
  -o lab2.log ./parent

clean:
	rm -f parent child lab2.log