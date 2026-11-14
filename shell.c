#include <stdio.h>

#define BUF_SIZE 1024

void evaluate_command(char **argv) {

}

int main(int argc, char **argv) {
    char buf[BUF_SIZE] = { 0 };
    int should_close = 0;
    while (!should_close) {
        fputs("> ", stdout);
        fgets(buf, BUF_SIZE, stdin);
        evaluate_command(argv);
    }
}
