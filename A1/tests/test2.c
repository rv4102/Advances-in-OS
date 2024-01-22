#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

void execute(int val[], int n, int pid) {
    int fd = open("/proc/partb_1_20CS30045_20CS30024", O_RDWR);
    char c = (char)n;
    write(fd, &c, 1);

    char pr = (pid) ? 'P' : 'C';

    for (int i = 0; i < n; i++) {
        int ret = write(fd, &val[i], sizeof(int));
        printf("%c [Proc %d] Write: %d, Return: %d\n", pr, getpid(), val[i], ret);
        usleep(100);
    }
    for (int i = 0; i < n; i++) {
        int out;
        int ret = read(fd, &out, sizeof(int));
        printf("%c [Proc %d] Read: %d, Return: %d\n", pr, getpid(), out, ret);
        usleep(100);
    }
    close(fd);
}

int main() {
    int val_p[] = {0, 1, -2, 3};
    int val_c[] = {6, -7, 8, -9};

    int pid = fork();
    if (pid == 0) {
        execute(val_c, 4, pid);
    } else {
        execute(val_p, 4, pid);
        wait(NULL);
    }
    
    return 0;
}