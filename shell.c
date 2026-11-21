#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define BUF_SIZE 1024
#define LIST 1
#define ALL 2
#define BOTH 3

char *get_current_working_directory(void) {
    char *ptr = getcwd(NULL, 0);
    return ptr;
}

void pwd(void) {
    char *cwd = get_current_working_directory();
    char full_path[1024] = { 0 };
    printf("%s\n", realpath(cwd, full_path));
    free(cwd);

}

int len_args(char **args) {
    int count = 0;
    for (int i = 0; args[i] != NULL; i++) {
        count++;
    }

    return count;
}

int len_pipe(char **input) {
    // starts at 1, this is used as amount parts in the pipe
    int count = 1;
    for (int i = 0; input[i] != NULL; i++) {
        if (strcmp(input[i], "|") == 0) {
            count++;
        }
    }

    return count;
}

void split_pipes(char **input, char **commands[]) {
    int n = 0;
    commands[n++] = &input[0];
    for (int i = 0; input[i] != NULL; i++) {
        if (strcmp(input[i], "|") == 0) {
            input[i] = NULL;
            commands[n++] = &input[i + 1];
        }
    }
}

int check_amt_digits(int number) {
    if (number < 10) {
        return 1;
    }

    return 1 + check_amt_digits(number/10);
}

void bitwise_print(struct stat *stat, int constant, char char_to_print) {
    if (stat->st_mode & constant) {
        printf("%c", char_to_print);
    } else {
        printf("%c", '-');
    }
}

void list(struct dirent *dir) {
    struct stat stat = { 0 };
    char buf[1024] = { 0 };
    lstat(dir->d_name, &stat);
    struct tm time = { 0 };
    localtime_r(&stat.st_mtime, &time);

    if (S_ISDIR(stat.st_mode)) {
        printf("%c", 'd');
    } else {
        printf("%c", '-');
    }

    bitwise_print(&stat, S_IRUSR, 'r');
    bitwise_print(&stat, S_IWUSR, 'w');
    bitwise_print(&stat, S_IXUSR, 'x');

    for (int i = 0; i < 2; i++) {
        bitwise_print(&stat, S_IROTH, 'r');
        bitwise_print(&stat, S_IWOTH, 'w');
        bitwise_print(&stat, S_IXOTH, 'x');
    }
    int size_digits = check_amt_digits(stat.st_nlink);
    for(int j = size_digits; j < 4; j++) {
        printf(" ");
    }
    printf("%c%ld", ' ', stat.st_nlink);

    struct passwd *pw = getpwuid(stat.st_uid);
    struct group *gr = getgrgid(stat.st_gid);

    printf("%c%s", ' ', pw->pw_name);
    printf("%c%s", ' ', gr->gr_name);

    size_digits = check_amt_digits(stat.st_size);
    for(int m = size_digits; m < 9; m++) {
        printf(" ");
    }
    printf("%c%ld", ' ', stat.st_size);
    strftime(buf, sizeof(buf), "%b %d %H:%M", &time);
    printf("%c%s", ' ', buf);
    printf("%c%s", ' ', dir->d_name);
    printf("\n");
}

int get_flags(char **args) {
    optind = 1;
    int ch = 0;
    int flag = 0;
    while ((ch = getopt(len_args(args), args, "al")) != -1) {
        switch (ch) {
            case 'a':
                flag += ALL;
                break;
            case 'l':
                flag += LIST;
                break;
            default:
                break;
        }
    }

    return flag;
}

void print_dir(struct dirent *dir, int flag) {
    if (flag <= 1) {
        if (strcmp(dir->d_name, ".") == 0 || 
            strcmp(dir->d_name, "..") == 0 || 
            dir->d_name[0] == '.') {
            return;
        }
    }

    if (flag == 1 || flag == 3) {
        list(dir);
        return;
    }

    printf("%s\t", dir->d_name);
}

void print_dirs(struct dirent *dir, DIR *dirp, int flag) {
    while ((dir = readdir(dirp)) != NULL) {
        print_dir(dir, flag);
    }

    printf("\n");

}

void pipe_commands(char **commands[], int n) {
    int prev = -1;

    for (int i = 0; i < n; i++) {
        int fd[2]; 
        pid_t pid = 0;

        if (i < n - 1 && pipe(fd) != 0) {
            perror("pipe");
            return;
        }

        pid = fork();
        
        // if failed
        if (pid < 0) {
            perror("fork");
            return;
        }

        // if child
        if (pid == 0) {
            // if we're at the last element of the pipe
            if (prev != -1) {
                dup2(prev, STDIN_FILENO);
                close(prev);
            }
            if (i < n - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }
            // TODO: Check if its a builtin or not
            execvp(commands[i][0], commands[i]);
            perror(commands[i][0]);
            exit(EXIT_FAILURE);
        }

        // if parent
        if (prev != -1) {
            close(prev);
        }
        if (i < n - 1) {
            close(fd[1]);
            prev = fd[0];
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }
}

void ls(char **args) {
    struct dirent *dir = NULL;
    DIR *dirp;
    char *cwd;
    int must_free_cwd = 0;

    int flag = get_flags(args);

    if (args[optind] == NULL) {
        cwd = get_current_working_directory();
        dirp = opendir(cwd);
        must_free_cwd = 1;
    } else {
        dirp = opendir(args[optind]);
    }

    if (dirp == NULL) {
        if (must_free_cwd != 0) {
            free(cwd);
        }
        perror("Not a directory!");
        return;
    }

    print_dirs(dir, dirp, flag);

    closedir(dirp);
    if (must_free_cwd != 0) {
        free(cwd);
    }

}

void cd(char **args) {
    char *dest = args[1];
    if (args[1] == NULL) {
        dest = "/home";
    }
    int rc = chdir(dest);
    if (rc != 0) {
        perror("chdir() failed!");
    }
}

int is_in_dir(char* dir_path, char* target) {
    DIR *dir_p = opendir(dir_path);
    struct dirent *dir = { 0 };
    while ((dir = readdir(dir_p)) != NULL) {
        if ((strncmp(dir->d_name, target, 1024)) == 0) {
            return 1;
        }
    }

    return -1;
}
char *dir_in_PATH(char *target) {
    char *PATH = getenv("PATH");
    char *paths[1024] = { 0 };
    char *token;
    char found[1024];
    int i = 0;

    PATH[strcspn(PATH, "\n")] = '\0';

    for (;(token = strsep(&PATH, ":")); i++) {
        paths[i] = token;
    }

    paths[i - 1] = NULL;

    for (int j = 0; paths[j] != NULL; j++) {
        if ((is_in_dir(paths[j], target)) != -1) {
            snprintf(found, 1024, "%s/%s", paths[j], target);
            return strdup(found);
        }
    }

    return "Not found!";
}

void evaluate_command(char *buf) {
    char *token;
    char *input[256] = { 0 };
    int i = 0;
    int wstatus;
    pid_t pid;

    if (buf[0] == '\0' ||
        buf[0] == '\n') {
        return;
    }

    buf[strcspn(buf, "\n")] = '\0';

    for (;(token = strsep(&buf, " \n")); i++) {
        input[i] = token;
    }

    input[i] = NULL;

    char *command = input[0];

    int amount_of_pipes = len_pipe(input);
    char **pipe_input[amount_of_pipes];
    if (amount_of_pipes > 0) {
        i = 0;
        split_pipes(input, pipe_input);
        pipe_commands(pipe_input, amount_of_pipes);
        return;
    }

    if (strncmp(command, "cd", BUF_SIZE) == 0) {
        cd(input);
        return;
    }

    if (strncmp(command, "ls", BUF_SIZE) == 0) {
        ls(input);
        return;
    }

    if (strncmp(command, "pwd", BUF_SIZE) == 0) {
        pwd();
        return;
    }

    int background_process = 0;
    char *last = input[len_args(input) - 1];
    
    // remove ampersand, because it cannot be used a proper argument
    size_t n = strlen(last);
    if (n > 0 && last[n - 1] == '&') {
        background_process = 1;
        last[n - 1] = '\0';
        if (last[0] == '\0') {
            input[i - 1] = NULL;
        }
    }

    // if its not a built-in, we check path
    if (command[0] != '/' &&
        command[0] != '~' &&
       (command[0] != '.' &&
        command[1] != '/')) {
        char *path_checked = dir_in_PATH(command);
        if ((strncmp(path_checked, "Not found", 1024)) != 0) {
            command = path_checked;
        }
    }

    pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        return;
    }

    if (pid == 0) {
        if (execv(command, input) < 0) {
            perror("execv() failed");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    if (background_process) {
        return;
    }

    wait(&wstatus);
    return;
}

int main(void) {
    char buf[BUF_SIZE] = { 0 };
    int should_close = 0;
    char *cwd = get_current_working_directory();
    fprintf(stderr, "%s\n", cwd);

    while (!should_close) {
        fputs("> ", stdout);
        fflush(stdout);
        fgets(buf, BUF_SIZE, stdin);
        evaluate_command(buf);
    }
}
