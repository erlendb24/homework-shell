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
#define READ 0
#define WRITE 1

char *get_current_working_directory(void) {
    char *ptr = getcwd(NULL, 0);
    return ptr;
}

int is_in_dir(char* dir_path, char* target) {
    DIR *dir_p = opendir(dir_path);
    if (dir_p == NULL) {
        return -1;
    }
    struct dirent *dir = { 0 };
    while ((dir = readdir(dir_p)) != NULL) {
        if ((strncmp(dir->d_name, target, 1024)) == 0) {
            closedir(dir_p);
            return 1;
        }
    }

    closedir(dir_p);
    return -1;
}

// need to free what this returns
char *dir_in_PATH(char *target) {
    char *PATH = getenv("PATH");
    if (PATH == NULL) {
        return NULL;
    }
    char *copy = strdup(PATH);
    // make rest point to copy, so that we can free the whole copy
    char *rest = copy;

    char *paths[1024] = { 0 };
    char *token;
    char found[1024];
    char *result = NULL;
    int i = 0;

    rest[strcspn(rest, "\n")] = '\0';

    for (;(token = strsep(&rest, ":")); i++) {
        paths[i] = token;
    }

    for (int j = 0; paths[j] != NULL; j++) {
        if ((is_in_dir(paths[j], target)) != -1) {
            snprintf(found, 1024, "%s/%s", paths[j], target);
             result = strdup(found);
             break;
        }
    }
    free(copy);
    return result;
}

// need to free what this returns
char *get_path_if_in_PATH(char *command) {
    char *path_checked = NULL;
    if (command[0] != '/' &&
        command[0] != '~' &&
       (command[0] != '.' &&
        command[1] != '/')) {
        path_checked = dir_in_PATH(command);
        if (path_checked != NULL) {
            command = strdup(path_checked);
            free(path_checked);
            return command;
        }
    }

    return NULL;
}

// need to free what this returns
char *get_home_directory(void) {
    uid_t user_id = getuid();
    struct passwd *user_passwd = getpwuid(user_id);
    char *home_dir = strdup(user_passwd->pw_dir);
    char *full_path = realpath(home_dir, NULL);
    free(home_dir);
    return full_path;
}

int check_if_builtin(char *command) {
    if (strncmp(command, "cd", BUF_SIZE) == 0 ||
        strncmp(command, "ls", BUF_SIZE) == 0 ||
        strncmp(command, "pwd", BUF_SIZE) == 0 ||
        strncmp(command, "last", BUF_SIZE) == 0) {
        return 1;
    }

    return 0;
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

void list(struct dirent *dir, char *dir_path) {
    char absolute_path[1024];
    snprintf(absolute_path, 1024, "%s/%s", dir_path, dir->d_name);
    struct stat stat = { 0 };
    char buf[1024] = { 0 };
    if (lstat(absolute_path, &stat) != 0) {
        perror(absolute_path);
        return;
    }
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

    struct passwd *pw = getpwuid(stat.st_uid);
    struct group *gr = getgrgid(stat.st_gid);

    printf("%c%s", ' ', pw->pw_name);
    printf("%c%s", ' ', gr->gr_name);

    int size_digits = check_amt_digits(stat.st_size);
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
    int ch, flag = 0;
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

void print_dir(char *dir_path, struct dirent *dir, int flag) {
    if (flag <= LIST) {
        if (strcmp(dir->d_name, ".") == 0 || 
            strcmp(dir->d_name, "..") == 0 || 
            dir->d_name[0] == '.') {
            return;
        }
    }

    if (flag == LIST || flag == BOTH) {
        list(dir, dir_path);
        return;
    }

    printf("%s\t", dir->d_name);
}

void print_dirs(char *dir_path, DIR *dirp, int flag) {
    struct dirent *dir;
    while ((dir = readdir(dirp)) != NULL) {
        print_dir(dir_path, dir, flag);
    }

    printf("\n");

}

void ls(char **args) {
    DIR *dirp;
    char *cwd;
    char *dir_path;
    int must_free_cwd = 0;

    // uses getopt() and moves optind
    int flag = get_flags(args);

    if (args[optind] == NULL) {
        cwd = get_current_working_directory();
        dir_path = cwd;
        must_free_cwd = 1;
    } else {
        dir_path = args[optind];
    }

    dirp = opendir(dir_path);
    if (dirp == NULL) {
        if (must_free_cwd != 0) {
            free(cwd);
        }
        perror("Not a directory!");
        return;
    }

    print_dirs(dir_path, dirp, flag);

    closedir(dirp);
    if (must_free_cwd != 0) {
        free(cwd);
    }

}

void pwd(void) {
    char *cwd = get_current_working_directory();
    char full_path[1024] = { 0 };
    printf("%s\n", realpath(cwd, full_path));
    free(cwd);

}

void cd(char **args) {
    char *dest = args[1];
    int rc = 0;
    // if no argument, go to home
    if (args[1] == NULL) {
        char *home_dir = get_home_directory();
        chdir(home_dir);
        free(home_dir);
        // use rc here, don't wanna duplicate free() call
        if (rc != 0) {
            perror("chdir");
        }
        return;
    }

    if (chdir(dest) != 0) {
        perror("chdir");
    }
}

void run_builtin(char *command, char **args, int *previous_result) {
    if (strncmp(command, "cd", BUF_SIZE) == 0) {
        cd(args);
        return;
    }

    if (strncmp(command, "ls", BUF_SIZE) == 0) {
        ls(args);
        return;
    }

    if (strncmp(command, "pwd", BUF_SIZE) == 0) {
        pwd();
        return;
    }

    if (strncmp(command, "last", BUF_SIZE) == 0) {
        printf("Last command returned: %d\n", *previous_result);
        return;
    }
}


void pipe_commands(char **commands[], int n, 
                   int background, int *previous_result) {
    int wstatus = 0;
    int prev = -1;
    int last_elem = n - 1;
    pid_t pids[64];

    for (int i = 0; i < n; i++) {
        int fd[2]; 
        pid_t pid = 0;
        char *first_command = commands[i][0];

        if (i < last_elem && pipe(fd) != 0) {
            perror("pipe");
            break;
        }

        pid = fork();
        
        // if failed
        if (pid < 0) {
            perror("fork");
            break;
        }

        // if child
        if (pid == 0) {
            pids[i] = pid;
            // if we're not at the first element of the pipe
            if (prev != -1) {
                dup2(prev, STDIN_FILENO);
                close(prev);
            }
            // if we're not at the last element of the pipe
            if (i < last_elem) {
                dup2(fd[WRITE], STDOUT_FILENO);
                close(fd[READ]);
                close(fd[WRITE]);
            }
            if (check_if_builtin(first_command)) {
                run_builtin(first_command, commands[i], previous_result);
                fflush(stdout);
                _exit(EXIT_SUCCESS);
            } else {
                first_command = get_path_if_in_PATH(first_command);
                execv(first_command, commands[i]);
                perror(first_command);
                _exit(EXIT_FAILURE);
            }
        }

        // if parent
        if (prev != -1) {
            close(prev);
        }
        if (i < last_elem) {
            close(fd[WRITE]);
            prev = fd[READ];
        }
    }
    if (prev != -1) {
        close(prev);
    }

    if (!background) {
        for (int i = 0; i < n; i++) {
            wait(&wstatus);
            waitpid(pids[i], &wstatus, 0);
            *previous_result = WEXITSTATUS(wstatus);
        }
    }
}


void evaluate_command(char *buf, int *previous_result) {
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

    int arguments_for_pipe = len_pipe(input);
    char **pipe_input[arguments_for_pipe];
    if (arguments_for_pipe > 1) {
        i = 0;
        split_pipes(input, pipe_input);
        pipe_commands(pipe_input, arguments_for_pipe, 
                      background_process, previous_result);
        return;
    }

    if (check_if_builtin(command) != 0) {
        run_builtin(command, input, previous_result);
        return;
    }
    

    // if its not a built-in, we check path

    char *path_command; //can be NULL

    path_command = get_path_if_in_PATH(command);
    if (path_command != NULL) {
        command = path_command;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        return;
    }

    if (pid == 0) {
        execv(command, input);
        perror("execv() failed");
        exit(EXIT_FAILURE);
    }

    if (background_process) {
        return;
    }

    wait(&wstatus);
    *previous_result = WEXITSTATUS(wstatus);
    
    free(path_command);
    return;
}

int main(void) {
    char buf[BUF_SIZE] = { 0 };
    int should_close = 0;
    int previous_result = 0;

    while (!should_close) {
        fputs("> ", stdout);
        fflush(stdout);
        if (fgets(buf, BUF_SIZE, stdin) == NULL) {
            break;
        }
        evaluate_command(buf, &previous_result);
    }
}
