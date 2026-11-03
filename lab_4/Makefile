all: parent child

parent: parent.c
	gcc -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L parent.c -o parent

child: child.c
	gcc -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L child.c -o child

strace_lab1: all
	strace -f -o lab1.log ./parent

strace_lab_current: all
	strace -f -yy -s 128 \
	  -e trace=execve,clone,fork,dup2,dup3,openat,read,write,close,wait4,exit_group,mmap,munmap,ftruncate,fstat,newfstatat,unlink,unlinkat,kill,rt_sigaction,rt_sigprocmask,rt_sigtimedwait \
	  -o lab4.log ./parent

clean:
	rm -f parent child lab4.log