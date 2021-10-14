#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

int child_alive = 0;
void trap_child(int sig)
{
    child_alive = 0;
}

int spawncmd(char *cmd)
{
    int fds[2];
    if (pipe(fds) == -1) return -1;
    pid_t pid = fork();
    if (pid == -1) return -1;

    if (pid == 0) {
        char *shell_cmd[] = {
            "/bin/sh",
            "-c",
            cmd,
            NULL
        };
        close(fds[STDOUT_FILENO]);
        dup2(fds[STDIN_FILENO], STDIN_FILENO);
        dup2(STDERR_FILENO, STDOUT_FILENO);

        if (execv(shell_cmd[0], shell_cmd)) {
            perror("exec child");
            exit(127);
        }
    }
    close(fds[STDIN_FILENO]);
    return fds[STDOUT_FILENO];
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Syntax: %s <cmd>", argv[0]);
        return 127;
    }

    int r;
    size_t buffsize = 4096;
    char buff[buffsize];

    signal(SIGCHLD, trap_child);
    signal(SIGPIPE, trap_child);

    int fd = spawncmd(argv[1]);
    if (fd == -1) return 127;
    child_alive = 1;

    while ((r = read(STDIN_FILENO, buff, buffsize)) > 0) {
        if (write(STDOUT_FILENO, buff, r) == -1) return 126;
        if (child_alive)
            if (write(fd, buff, r) == -1)
                child_alive = 0;
    }
    int status;
    if (child_alive) close(fd);
    if (wait(&status) == -1) return 124;
    // TODO check if exited/signaled
    return WEXITSTATUS(status);
}
