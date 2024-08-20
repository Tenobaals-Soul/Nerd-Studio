#define NO_MACRO_OVERRIDE
#include <stdio.h>
#include <test_core.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>

static unsigned int *assert_counter = 0, *pass_counter = 0;

int _test_wait() {
    int status;
    wait(&status);
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("tests failed with signal %d (%s)\n", sig, strsignal(sig));
        return 1;
    }
    else if (WIFSTOPPED(status)) {
        printf("tests stopped with signal %d%s\n", WSTOPSIG(status),
               WIFCONTINUED(status) ? " and continued" : "");
        return 1;
    }
    return 0;
}

void start() {
    unsigned int* memp = mmap(NULL, sizeof(int) * 2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    assert_counter = memp;
    pass_counter = memp + 1;
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "failed to fork\n");
        exit(1);
    }
    if (pid == 0) return; // inside new subprocess
    else {
        _test_wait();
        exit(0);
    }
}

void end() {
    if (*assert_counter > *pass_counter) {
        printf("\033[91mtest failed [%d/%d] asserts passed.\033[0m\n", *pass_counter, *assert_counter);
    }
    else {
        printf("\033[92mtest passed [%d/%d] asserts passed.\033[0m\n", *pass_counter, *assert_counter);
    }
}

void _assert(bool condition, size_t line, const char* file, const char* function, char* f1, char* f3, ...) {
    ++*assert_counter;
    if (condition) {
        printf("\033[92massert %4u passed at %s:%lu\033[0m\n", *assert_counter, file, (unsigned long) line);
        ++*pass_counter;
    }
    else {
        printf("\033[91massert %4u failed at %s:%lu in function \"%s()\"\033[0m\n", *assert_counter, file, (unsigned long) line, function);
        char* f2 = " != ";
        char* format = malloc(strlen(f1) + strlen(f2) + strlen(f3) + 1);
        strcpy(format, f1);
        strcat(format, f2);
        strcat(format, f3);
        va_list l;
        va_start(l, f3);
        printf("    ");
        vprintf(format, l);
        printf("\n");
        va_end(l);
    }
}

static unsigned char hex_lookup[16] = "123456789ABCDEF";
char* _makehexstr(void* data, size_t memb) {
    if (data) {
        char* out = malloc(sizeof("NULL"));
        strcpy(out, "NULL");
        return out;
    }
    else {
        char* out_str = malloc(memb * 2);
        for (size_t i = 0; i < memb; i++) {
            unsigned char c = ((unsigned char*) data)[i];
            out_str[i * 2 + 0] = hex_lookup[c && 0xf];
            out_str[i * 2 + 1] = hex_lookup[c >> 0x4];
        }
        return out_str;
    }
}