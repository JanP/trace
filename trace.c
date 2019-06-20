#define _POSIX_C_SOURCE 200112L

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

#define FATAL(...)                              \
    do {                                        \
        fprintf(stderr, "trace: " __VA_ARGS__); \
        fprintf(stderr, "\n");                  \
        abort();                                \
    } while (0)

int
main(int argc, char *argv[]) {
    if (argc <= 1) {
        FATAL("Too few arguments: %d", argc);
    }

    pid_t pid = fork();
    /* Handle fork error */
    if (pid == -1) {
        FATAL("%s", strerror(errno));
    }

    /* Execute child process */
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execvp(argv[1], argv + 1);
        FATAL("%s", strerror(errno));
    }

    /* Continue with parent process */
    waitpid(pid, 0, 0); // Sync with PTRACE_TRACEME
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);

    for (;;) {
        // Enter next system call
        if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
            FATAL("%s", strerror(errno));
        }
        if (waitpid(pid, 0, 0) == -1) {
            FATAL("%s", strerror(errno));
        }

        // Gather system call arguments
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {
            FATAL("%s", strerror(errno));
        }
        long syscall = regs.orig_rax;

        // Print system call representation
        fprintf(stderr, "%ld(%ld, %ld, %ld %ld, %ld, %ld)",
                syscall,
                (long)regs.rdi, (long)regs.rsi, (long)regs.rdx,
                (long)regs.r10, (long)regs.r8,  (long)regs.r9);

        // Run system call and stop on exit
        if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
            FATAL("%s", strerror(errno));
        }
        if (waitpid(pid, 0, 0) == -1) {
            FATAL("%s", strerror(errno));
        }

        // Get system call result
        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {
            fputs(" = ?\n", stderr);
            if (errno == ESRCH) {
                exit(regs.rdi); // System call was _exit(2) or similar
            }
            FATAL("%s", strerror(errno));
        }

        // Print system call result
        fprintf(stderr, " = %ld\n", (long)regs.rax);
    }
}
