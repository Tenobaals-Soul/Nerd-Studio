#ifndef INCLUDE_TEST_CORE_H
#define INCLUDE_TEST_CORE_H
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>

void start();
void end();

char* _makehexstr(void* data, size_t memb);
void _assert(bool condition, size_t line, const char* file, const char* function, char* f1, char* f3, ...);
int _test_wait();

#define STR(x) #x

#define launch(code) do { \
    printf("start section %s\n", STR(code)); \
    pid_t pid = fork(); \
    if (pid == 0) {code;} \
    else { \
        if (_test_wait()) _assert(1 == 0, __LINE__, __FILE__, "???\", \"trying to launch: \""STR(code)"\" with macro: launch", \
            "%s", "%s", "launch()", "successful"); \
        exit(0); \
     } \
} while(0)

#define EVAL_TYPE_FORMATER(exp) (_Generic((exp), \
        char: "%d", \
        short: "%d", \
        int: "%d", \
        long: "%ld", \
        long long: "%lld", \
        unsigned char: "%u", \
        unsigned short: "%u", \
        unsigned int: "%u", \
        unsigned long: "%lu", \
        unsigned long long: "%llu", \
        float: "%f", \
        double: "%f", \
        long double: "%lf", \
        default: "%p" \
    ))

#define assert_equal(x, y) do {\
    typeof(x) evalx = x;\
    typeof(y) evaly = y;\
    _assert(evalx == evaly, __LINE__, __FILE__, __func__,\
        (EVAL_TYPE_FORMATER(evalx)), EVAL_TYPE_FORMATER(evaly), evalx, evaly\
    );\
} while (0)

#define FORMAT_STRING(str) str ? "\"": "", str ? str : "NULL", str ? "\"" : ""
#define assert_str_equal(x, y) do {\
    const char* evalx = x;\
    const char* evaly = y;\
    _assert(evalx == evaly || (evalx && evaly && strcmp(evalx, evaly) == 0), __LINE__, __FILE__, __func__,\
        "%s%s%s", "%s%s%s", FORMAT_STRING(evalx), FORMAT_STRING(evaly)\
    );\
} while (0)

#define assert_true(x) do {\
    bool evalx = x;\
    _assert(evalx, __LINE__, __FILE__, __func__,\
        EVAL_TYPE_FORMATER(evalx), "true", evalx\
    );\
} while (0)

#define assert_memeq(x, y, memb) do {\
    void* evalx = x;\
    void* evaly = y;\
    char* hex_str1 = _makehexstr(x, memb);\
    char* hex_str2 = _makehexstr(y, memb);\
    bool condition = evalx == evaly || (evalx && evaly && memcmp(evalx, evaly, memb) == 0);\
    _assert(condition, __LINE__, __FILE__, __func__,\
        "%s", "%s", hex_str1, hex_str2\
    );\
    free(hex_str1);\
    free(hex_str2);\
} while(0)

#endif