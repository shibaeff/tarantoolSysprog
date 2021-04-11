#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>


#include "structs.h"

int choose(cmd_arr *cmd_list, int *cur_ch);

void make_chdir(cmd_arr *cmd_list, const comandlet *first_cmd);

int catch_exit_chdir(cmd_arr *cmd_list);

void make_chdir(cmd_arr *cmd_list, const comandlet *first_cmd);

void sh_loop(struct cmd_arr *cmd_list);


void ignore_spaces(int *cur_ch);

void ignore_comment(int *cur_ch);

char *get_quotes(int *cur_ch);

char *get_word(int *cur_ch);

void get_arg(int *ch, comandlet *cur_cmd);

void get_file(int *ch, comandlet *cur_cmd);

void push_back(string *str, char ch);

void new_cmd(cmd_arr *cmd_list);

void delete_commands(cmd_arr *cmd_list);

int main()
{
    cmd_arr *cmd_list = calloc(sizeof(*cmd_list), 1);
    new_cmd(cmd_list);
    int cur_ch = getchar();
    while (cur_ch != EOF) {
        ignore_spaces(&cur_ch);
        cur_ch = choose(cmd_list, &cur_ch);
    }
    delete_commands(cmd_list);
    free(cmd_list);
    return 0;
}

int choose(cmd_arr *cmd_list, int *cur_ch)
{
    switch ((*cur_ch)) {
        case EOF:
            break;
        case '\n':
            sh_loop(cmd_list);
            (*cur_ch) = getchar();
            break;
        case '#':
            ignore_comment(cur_ch);
            break;
        case '>':
            get_file(cur_ch, &cmd_list->comandlet_buf[cmd_list->count - 1]);
            break;
        case '|':
            new_cmd(cmd_list);
            (*cur_ch) = getchar();
            break;
        default:
            get_arg(cur_ch, &cmd_list->comandlet_buf[cmd_list->count - 1]);
            break;
    }
    return (*cur_ch);
}

int catch_exit_chdir(cmd_arr *cmd_list)
{
    if (cmd_list->count > 1) {
        return 0;
    }
    comandlet *first_cmd = &cmd_list->comandlet_buf[0];
    if (!strcmp("exit", first_cmd->args[0])) {
        exit(0);
    }

    if (!strcmp("cd", first_cmd->args[0])) {
        make_chdir(cmd_list, first_cmd);
        return 1;
    }

    return 0;
}

void make_chdir(cmd_arr *cmd_list, const comandlet *first_cmd)
{
    int chdir_res;
    if (first_cmd->argc == 1) {
        chdir_res = chdir(getenv("HOME"));
    } else {
        chdir_res = chdir(first_cmd->args[1]);
    }

    if (chdir_res < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
    }
    delete_commands(cmd_list);
    new_cmd(cmd_list);
}

void sh_loop(struct cmd_arr *cmd_list)
{
    if (!cmd_list->comandlet_buf[0].argc) {
        return;
    }
    if (catch_exit_chdir(cmd_list)) {
        return;
    }

    int (*pipefd)[2] = calloc(cmd_list->count - 1, sizeof(int[2]));

    for (int i = 0; i < cmd_list->count; ++i) {
        if (i != cmd_list->count - 1)
            throw(pipe(pipefd[i]) == 0);

        comandlet *cur_cmd = &cmd_list->comandlet_buf[i];

        pid_t pid = fork();
        throw(pid >= 0);
        if (!pid) {
            if (i > 0) {
                dup2(pipefd[i - 1][0], 0);
                close(pipefd[i - 1][0]);
            }
            if (i < cmd_list->count - 1) {
                dup2(pipefd[i][1], 1);
                close(pipefd[i][1]);
            }

            if (cur_cmd->out) {
                int file_fd = open(cur_cmd->out->filename, cur_cmd->out->mode, 0666);
                dup2(file_fd, 1);
                close(file_fd);
            }

            cur_cmd->args = realloc(cur_cmd->args, (cur_cmd->argc + 1) * sizeof(char *));
            throw(cur_cmd->args);

            cur_cmd->args[cur_cmd->argc] = NULL;

            execvp(cur_cmd->args[0], cur_cmd->args);
            exit(1);
        }

        if (i != cmd_list->count - 1) {
            close(pipefd[i][1]);
        }
        if (i != 0) {
            close(pipefd[i - 1][0]);
        }
    }

    free(pipefd);

    for (int i = 0; i < cmd_list->count; ++i)
        wait(NULL);

    delete_commands(cmd_list);
    new_cmd(cmd_list);
}

void ignore_spaces(int *cur_ch)
{
    while (*cur_ch != '\n' && isspace(*cur_ch))
        *cur_ch = getchar();
}

void ignore_comment(int *cur_ch)
{
    while (*cur_ch != '\n' && *cur_ch != EOF)
        *cur_ch = getchar();
}

char *get_quotes(int *cur_ch)
{
    int quote_ch = *cur_ch;

    string word = {NULL, 0, 0};
    int backslash = 0;

    while ((*cur_ch = getchar()) != EOF) {
        if (backslash) {
            if (*cur_ch != quote_ch) {
                push_back(&word, '\\');
            }
            if (*cur_ch != '\\') {
                push_back(&word, *cur_ch);
            }
            backslash = 0;
        } else {
            if (*cur_ch == quote_ch) {
                *cur_ch = getchar();
                break;
            }
            if (*cur_ch == '\\') {
                backslash = 1;
            } else {
                push_back(&word, *cur_ch);
            }
        }
    }
    word.chr = realloc(word.chr, (word.sz + 1) * sizeof(char));
    word.chr[word.sz] = 0;

    return word.chr;
}

char *get_word(int *cur_ch)
{
    if (*cur_ch == '\'' || *cur_ch == '\"') {
        return get_quotes(cur_ch);
    }

    string word = {NULL, 0, 0};
    int backslash = 0;

    do {
        if (backslash) {
            if (*cur_ch != '\n') {
                push_back(&word, *cur_ch);
            }
            backslash = 0;
        } else {
            if (*cur_ch == '\\') {
                backslash = 1;
            } else if (isspace(*cur_ch) || *cur_ch == '|') {
                break;
            } else {
                push_back(&word, *cur_ch);
            }
        }

    } while ((*cur_ch = getchar()) != EOF);

    if (!word.sz) {
        return NULL;
    }

    word.chr = realloc(word.chr, (word.sz + 1) * sizeof(char));
    word.chr[word.sz] = 0;

    return word.chr;
}

void get_arg(int *ch, comandlet *cur_cmd)
{
    char *arg = get_word(ch);
    if (!arg) {
        return;
    }

    cur_cmd->argc++;
    cur_cmd->args = realloc(cur_cmd->args, cur_cmd->argc * sizeof(char *));
    throw(cur_cmd->args);

    cur_cmd->args[cur_cmd->argc - 1] = arg;
}

void get_file(int *ch, comandlet *cur_cmd)
{
    int mode = O_WRONLY | O_CREAT | O_TRUNC;

    switch (*ch = getchar()) {
        case EOF:
            return;
        case '>':
            mode ^= O_TRUNC;
            mode |= O_APPEND;
            *ch = getchar();
                    __attribute__ ((fallthrough));
        default:
            ignore_spaces(ch);
    }

    if (*ch == EOF || *ch == '\n') {
        return;
    }

    char *filename = get_word(ch);
    if (!cur_cmd->out) {
        cur_cmd->out = calloc(1, sizeof(file));
        throw(cur_cmd->out);
    } else {
        free(cur_cmd->out->filename);
    }

    cur_cmd->out->filename = filename;
    cur_cmd->out->mode = mode;
}

void push_back(string *str, char ch)
{
    if (str->sz >= str->cap) {
        str->cap = str->cap * 2 + 1;
        str->chr = realloc(str->chr, str->cap * sizeof(char));
        throw(str->chr);
    }
    str->chr[str->sz++] = ch;
}

void new_cmd(cmd_arr *cmd_list)
{
    cmd_list->count++;
    cmd_list->comandlet_buf = realloc(cmd_list->comandlet_buf, cmd_list->count * sizeof(comandlet));
    throw(cmd_list->comandlet_buf);

    memset(&cmd_list->comandlet_buf[cmd_list->count - 1], 0, sizeof(comandlet));
}

void delete_commands(cmd_arr *cmd_list)
{
    for (int i = 0; i < cmd_list->count; ++i) {
        comandlet *cur_cmd = &cmd_list->comandlet_buf[i];
        for (int j = 0; j < cur_cmd->argc; ++j)
            free(cur_cmd->args[j]);

        if (cur_cmd->out) {
            free(cur_cmd->out->filename);
            free(cur_cmd->out);
        }

        free(cur_cmd->args);
    }

    free(cmd_list->comandlet_buf);
    memset(cmd_list, 0, sizeof(*cmd_list));
}
