#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

#define BUF_SIZE 1024
#define ALL 1
#define LIST 2
#define BOTH 3

char *get_current_working_directory(void) {
    char *ptr = getcwd(NULL, 0);
    return ptr;
}

void list(void) {
}

int get_flags(char **args) {
    int ch = 0;
    int flag = 0;
    while ((ch = getopt(2, args, "al")) != -1) {
        switch (ch) {
            case 'a':
                flag += ALL;
            case 'l':
                flag += LIST;
                break;
            default:
                break;
        }
    }

    return flag;
}

void ls(char **args) {
    struct dirent *dir;
    DIR *dirp;
    char *cwd;
    int must_free_cwd = 0;

    int flag = get_flags(args);

    if (strcmp(args[1], "a") > 0) {
        cwd = get_current_working_directory();
        dirp = opendir(cwd);
        must_free_cwd = 1;
    } else {
        dirp = opendir(args[1]);
    }

    if (dirp == NULL) {
        if (must_free_cwd != 0) {
            free(cwd);
        }
        perror("Not a directory!");
        return;
    }


    while ((dir = readdir(dirp)) != NULL) {
        if (flag < 1) {
            if (strcmp(dir->d_name, ".") == 0 || 
                strcmp(dir->d_name, "..") == 0) {
                continue;
            }
        }
        if (flag >= 2) {
            list();
            break;
        }
        printf("%s", dir->d_name);
        printf("\t");
    }

    printf("\n");

    if (must_free_cwd != 0) {
        free(cwd);
    }
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
    printf("%d\n", i);

    char *command = input[0];

    for (int j = 0; input[j] != NULL; j++) {
        printf("%d: %s\n", j, input[j]);
    }

    if (strncmp(command, "cd", BUF_SIZE) == 0) {
        cd(input);
        return;
    }

    if (strncmp(command, "ls", BUF_SIZE) == 0) {
        ls(input);
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
