#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

enum {
    INITIAL_SZ = 1
};

#define CORO_LOCAL_DATA struct {                \
    int deep;                        \
    int* arr;                    \
    FILE *fp;                          \
    int current_size;\
    int current_index;\
    int current;                                \
    char *name;                                 \
    int *size;                                                \
}



#include "coro_jmp.h"

/*
 * simple quicksort implementation
 * I always take it from R. Sadgewick's book
 */
void quicksort(int *number, int first, int last)
{
    int i, j, pivot, temp;

    if (first < last) {
        pivot = first;
        i = first;
        j = last;

        while (i < j) {
            while (number[i] <= number[pivot] && i < last)
                i++;
            while (number[j] > number[pivot])
                j--;
            if (i < j) {
                temp = number[i];
                number[i] = number[j];
                number[j] = temp;
            }
        }

        temp = number[pivot];
        number[pivot] = number[j];
        number[j] = temp;
        quicksort(number, first, j - 1);
        quicksort(number, j + 1, last);

    }
}

/* how to user this wonderful shortcut
 *   check(mapped == MAP_FAILED, "mmap %s failed: %s",
          file_name, strerror(errno));
*/
static void
check(int test, const char *message, ...)
{
    if (test) {
        va_list args;
        va_start(args, message);
        vfprintf(stderr, message, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }
}


/*
 * read and sort the single file
 */

int *
sortfile(char *name, int *size)
{
    FILE *fp = fopen(name, "r");
    int *arr = malloc(INITIAL_SZ * sizeof(int));
    int current_size = INITIAL_SZ;
    int current_index = 0;
    int current;
    while (fscanf(fp, "%d", &current) == 1) {
        if (current_size == current_index) {
            arr = realloc(arr, sizeof(int) * current_size * 2);
            current_size *= 2;
        }
        arr[current_index++] = current;
    }
    // no need for the read decsriptors
    fclose(fp);
    quicksort(arr, 0, current_index - 1);
    *size = current_index;
    return arr;
    // yield the file
    // open for the write and truncate
}

int *
coro_sortfile()
{
    coro_this()->fp = fopen(coro_this()->name, "r");
    coro_yield();
    coro_this()->arr = malloc(INITIAL_SZ * sizeof(int));
    coro_yield();
    coro_this()->current_size = INITIAL_SZ;
    coro_yield();
    while (fscanf(coro_this()->fp, "%d", &coro_this()->current) == 1) {
        coro_yield();
        printf("ok\n");
        if (coro_this()->current_size == coro_this()->current_index) {
            coro_this()->arr = realloc(coro_this()->arr, sizeof(int) * coro_this()->current_size * 2);
            coro_this()->current_size *= 2;
        }
        coro_this()->arr[coro_this()->current_index++] = coro_this()->current;
    }
    // no need for the read decsriptors
    fclose(coro_this()->fp);
    coro_yield();
    quicksort(coro_this()->arr, 0, coro_this()->current_index - 1);
    coro_yield();
    *coro_this()->size = coro_this()->current_index;
    coro_yield();
    // yield the file
    // open for the write and truncate
}

int
main(int argc, char **argv)
{
    // ini coroutines
    coro_count = argc - 1;
    coros = malloc(coro_count * sizeof(struct coro));
    for (int i = 0; i < coro_count; ++i) {
        if (coro_init(&coros[i]) != 0) {
            break;
        }
    }
    int fd = open("test1.txt", O_RDONLY);

    int *sorted[argc - 1];
    int sizes[argc - 1];
    int iterators[argc - 1];
    int overall = 0;
    for (int i = 1; i < argc; ++i) {
        iterators[i - 1] = 0;
        coros[i - 1].name = argv[i];
        coros[i - 1].size = &sizes[i - 1];
//        sorted[i - 1] = sortfile(argv[i], &sizes[i - 1]);
        coro_sortfile();
        sorted[i - 1] = coros[i - 1].arr;
        overall += sizes[i - 1];
    }

    FILE *out = fopen("output.txt", "w");
    // suboptimal k-way merge
    for (int i = 0; i < overall; ++i) {
        int value = INT_MAX;
        int index = -1;
        for (int k = 0; k < argc - 1; ++k) {
            if (iterators[k] < sizes[k] && sorted[k][iterators[k]] < value) {
                index = k;
                value = sorted[k][iterators[k]];
            }
        }
        fprintf(out, "%d ", sorted[index][iterators[index]++]);
    }
    fclose(out);
    free(coros);
//    for (int i = 1; i < argc; ++i) {
//        close(fds[i - 1]);
//    }
    return 0;
}