//#ifndef SHELL_PARSER_H
//#define SHELL_PARSER_H
#pragma once

#include <errno.h>
#include <stdio.h>

typedef struct file {
    char *filename;
    int mode;
} file;

typedef struct comandlet {
    char **args;
    int argc;
    file *out;
} comandlet;

typedef struct cmd_arr {
    int count;
    comandlet *comandlet_buf;
} cmd_arr;

typedef struct string {
    char *chr;
    int sz;
    int cap;
} string;


#define throw(expr) do { \
    if (!(expr)) {                \
        fprintf(stderr, "%s\n", strerror(errno)); \
        exit(1);                              \
    }                               \
} while (0)

void push_back(string *str, char ch);

void new_cmd(cmd_arr *cmd_list);

void delete_commands(cmd_arr *cmd_list);

//#endif
