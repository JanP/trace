#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

// Standard C library
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// POSIX
#include <unistd.h>
#include <sys/user.h>
#include <sys/wait.h>

// Linux
#include <syscall.h>
#include <sys/ptrace.h>

#include <sys/uio.h>

#include "strace/kernel_types.h"

#define FATAL(...)                              \
    do {                                        \
        fprintf(stderr, "[trace]: " __VA_ARGS__); \
        fprintf(stderr, "\n");                  \
        exit(EXIT_FAILURE);                     \
    } while (0)

#define PATH_MAX					(256)

static int
ptrace_syscall(pid_t pid) {
	if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
		fprintf(stderr, "trace: ptrace: %s\n", strerror(errno));
		return -1;
	}

	if (waitpid(pid, 0, 0) == -1) {
		fprintf(stderr, "trace: waitpid: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static ssize_t
get_string(pid_t pid, void *src, void *dst, size_t size) {
	const struct iovec local[] = {
		{
			.iov_base = dst,
			.iov_len = size,
		},
	};
	const struct iovec remote[] = {
		{
			.iov_base = src,
			.iov_len = size,
		},
	};

	ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
	if (nread == -1) {
		fprintf(stderr, "trace: %s\n", strerror(errno));
		return -1;
	}

	return nread;
}

extern const char *syscall_name(kernel_ulong_t scno);

static int
ptrace_get_syscall(pid_t pid, kernel_ulong_t *syscall, struct user_regs_struct *regs) {
	if (ptrace(PTRACE_GETREGS, pid, 0, regs) == -1) {
		fprintf(stderr, "trace: %s\n", strerror(errno));
		return -1;
	}

	*syscall = regs->orig_rax;
	fprintf(stderr, "%s(%ld, %ld, %ld, %ld, %ld, %ld)",
		    syscall_name(*syscall),
		    (long)regs->rdi, (long)regs->rsi, (long)regs->rdx,
		    (long)regs->r10, (long)regs->r8,  (long)regs->r9);

	return 0;
}

static int
ptrace_get_syscall_res(pid_t pid, long *res) {
	struct user_regs_struct regs;
	if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {
		fprintf(stderr, " = ?\n");
		if (errno == ESRCH) {
			exit(regs.rdi);
		}
		fprintf(stderr, "trace: %s\n", strerror(errno));
	}

	fprintf(stderr, " = %ld\n", (long)regs.rax);
	*res = (long)regs.rax;

	return 0;
}

int
main(int argc, char *argv[]) {
    if (argc <= 1) {
        FATAL("Too few arguments: %d", argc);
    }

    pid_t pid = fork();
    /* Handle fork error */
    if (pid == -1) {
        FATAL("fork: %s", strerror(errno));
    }

    /* Execute child process */
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execvp(argv[1], argv + 1);
        FATAL("execvp: %s", strerror(errno));
    }

    /* Continue with parent process */
    waitpid(pid, 0, 0); // Sync with PTRACE_TRACEME
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);

    for (;;) {
        // Enter next system call
		if (ptrace_syscall(pid) == -1) {
			exit(EXIT_FAILURE);
		}

        // Gather system call arguments
        struct user_regs_struct regs;
		kernel_ulong_t syscall;
		if (ptrace_get_syscall(pid, &syscall, &regs) == -1) {
			exit(EXIT_FAILURE);
		}

		if (strcmp(syscall_name(syscall), "access") == 0) {
			char path[PATH_MAX];
			memset(path, '\0', sizeof(path));
			ssize_t nread = get_string(pid, (void *)regs.rdi, path, sizeof(path) - 1);
			if (nread == -1) {
				FATAL("get_string: %s", strerror(errno));
			}
			fprintf(stdout, "access(%s)\n", path);
		}

		if (strcmp(syscall_name(syscall), "openat") == 0) {
			char path[PATH_MAX];
			memset(path, '\0', sizeof(path));
			ssize_t nread = get_string(pid, (void *)regs.rsi, path, sizeof(path) - 1);
			if (nread == -1) {
				FATAL("get_string: %s", strerror(errno));
			}
			fprintf(stdout, "openat(%s)\n", path);
		}

        // Run system call and stop on exit
		if (ptrace_syscall(pid) == -1) {
			exit(EXIT_FAILURE);
		}

        // Get system call result
		long res;
		if (ptrace_get_syscall_res(pid, &res) == -1) {
			exit(EXIT_FAILURE);
		}
    }
}
