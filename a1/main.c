#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>

#include "fork.h"

int was_alarm=0;

void set_close_on_exec(int fd) {
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        f_error("fcntl failed");
    }
    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) < 0) {
        f_error("fcntl failed");
    }
}

void wait_for_pid(int pid, const char* process) {
    int status = 0;
    while (1) {
        int ret = wait(&status);
        if (ret == pid) {
            printf("Process %s finished with status %d\n", process, WEXITSTATUS(status));
            break;
        } else if (errno == EINTR) {
            fprintf(stderr, "marker: At least one process did not finish\n");
            ret = kill(pid, SIGKILL);
            if (ret == -1) {
                perror("kill failed");
            }
            exit(1);
        } else {
            exit(1);
        }
    }
}

/* The main program does much of the work. parses the command line arguments */
/* sets up the alarm and the alarm signal handler opens the files and pipes */
/* for redirection etc., invoke start_child, closes files that should be  */
/* closed, waits for the children to finish and reports their status */
/* or exits printing a message and kill its children if they do not die */
/* in time (just the parent exiting may not be enough to kill the children) */
int main(int argc, char *argv[])
{
    int fdin = open("test.in", O_RDONLY | O_CLOEXEC);
    int fdout = open("test.out", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, S_IRWXU|S_IRWXG|S_IRWXO);
    int ferror1 = open("test.err1", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, S_IRWXU|S_IRWXG|S_IRWXO);
    int ferror2 = open("test.err2", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, S_IRWXU|S_IRWXG|S_IRWXO);
    int pipefds[2];
    pipe(pipefds);
    set_close_on_exec(pipefds[0]);
    set_close_on_exec(pipefds[1]);

    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = alrm_handler;
    sigaction(SIGALRM, &sig, NULL);
    alarm(3);

    int i = 0;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p-") == 0) {
            break;
        }
    }
    argv[i] = NULL;
    char* process1 = argv[1];
    pid_t pid1 = start_child(argv[1], argv + 1, fdin, pipefds[1], ferror1);
    int j = 0;
    for (j = i + 1; j < argc; ++j) {
        argv[j-i] = argv[j];
    }
    argv[j-i] = NULL;
    char* process2 = argv[1];
    pid_t pid2 = start_child(argv[1], argv + 1, pipefds[0], fdout, ferror2);
    close(pipefds[1]);
    close(pipefds[0]);

    wait_for_pid(pid1, process1);
    wait_for_pid(pid2, process2);
}
