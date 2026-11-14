#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUF_SIZE 1024

char *get_current_working_directory(void) {
    char *ptr = getcwd(NULL, 0);
    return ptr;
}

void cd(char **args) {
    fprintf(stderr, "[%s]\n", args[1]);
    for (const char *p = args[1]; *p; p++)
        fprintf(stderr, "%02x ", (unsigned char)*p);
    fprintf(stderr, "\n");
    int rc = chdir(args[1]);
    if (rc != 0) {
        perror("chdir() failed!");
    }
}

void evaluate_command(char *buf) {
    char *token;
    char *input[256] = { 0 };
    int i = 0;
    int wstatus;
    pid_t pid;

    for (;(token = strsep(&buf, " \n")); i++) {
        input[i] = token;
    }

    input[i] = NULL;

    char *command = input[0];

    for (int j = 0; input[j] != NULL; j++) {
        printf("%d: %s\n", j, input[j]);
    }

    if (strncmp(command, "cd", BUF_SIZE) == 0) {
        cd(input);
        return;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork() failed!");
        return;
    }

    if (pid == 0) {
        execvp(command, input);
        exit(EXIT_SUCCESS);
    }

    wait(&wstatus);
    return;
}

int main(int argc, char **argv) {
    char buf[BUF_SIZE] = { 0 };
    int should_close = 0;
    char *cwd = get_current_working_directory();
    fprintf(stderr, "%s\n", cwd);

    while (!should_close) {
        fputs("> ", stdout);
        fgets(buf, BUF_SIZE, stdin);
        evaluate_command(buf);
    }
}
