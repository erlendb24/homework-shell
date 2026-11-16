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
    localtime_r(&stat.st_mtimespec.tv_sec, &time);

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
    printf("%c%d", ' ', stat.st_nlink);

    struct passwd *pw = getpwuid(stat.st_uid);
    struct group *gr = getgrgid(stat.st_gid);

    printf("%c%s", ' ', pw->pw_name);
    printf("%c%s", ' ', gr->gr_name);

    size_digits = check_amt_digits(stat.st_size);
    for(int m = size_digits; m < 9; m++) {
        printf(" ");
    }
    printf("%c%lld", ' ', stat.st_size);
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

    printf("%d", flag);
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

void ls(char **args) {
    for (int j = 0; args[j] != NULL; j++) {
        printf("%d: %s\n", j, args[j]);
    }

    struct dirent *dir = NULL;
    DIR *dirp;
    char *cwd;
    int must_free_cwd = 0;

    int flag = get_flags(args);
    printf("FLAG: %d", flag);

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

    buf[strcspn(buf, "\n")] = '\0';

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

    if (strncmp(command, "pwd", BUF_SIZE) == 0) {
        pwd();
        return;
    }

    // do this after built-ins, because they cannot return early anyway
    int background_process = 0;
    char *last = input[len_args(input) - 1];
    if (*last == '&') {
        background_process = 1;
    }

    // remove ampersand, because it cannot be used a proper argument
    *input[strcspn(last, "&")] = '\0';

    pid = fork();
    if (pid < 0) {
        perror("fork() failed!");
        return;
    }

    if (pid == 0) {
        execvp(command, input);
        exit(EXIT_SUCCESS);
    }

    if (background_process) {
        return;
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
