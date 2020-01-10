#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "fork.h"

/* This is the handler of the alarm signal. It just updates was_alarm */
void alrm_handler(int i)
{
    was_alarm = 1;
}

/* Prints string s using perror and exits. Also checks errno to make */
/* sure that it is not zero (if it is just prints s followed by newline */
/* using fprintf to the standard error */
void f_error(char *s)
{
    if (errno != 0) {
        perror(s);
        exit(-1);
    }
}

/* Creates a child process using fork and a function from the exec family */
/* The standard input output and error are replaced by the last three */
/* arguments to implement redirection and piping */
pid_t start_child(const char *path, char *const argv[],
		  int fdin, int fdout, int fderr)
{
    pid_t pid = fork();
    if (pid < 0) {
        f_error("fork failed");
    }
    if (pid == 0) {
        int ret = dup2(fdin, STDIN_FILENO);
        if (ret != 0) {
            f_error("dup2 failed");
        }
        ret = dup2(fdout, STDOUT_FILENO);
        if (ret != 0) {
            f_error("dup2 failed");
        }
        ret = dup2(fderr, STDERR_FILENO);
        if (ret != 0) {
            f_error("dup2 failed");
        }
        ret = execvp(path, argv);
        if (ret != 0) {
            f_error("execvp failed");
        }
    }
    return pid;
}
