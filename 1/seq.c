#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <poll.h>

enum {
    INITIAL_SZ = 1,
};

#define CORO_LOCAL_DATA struct {                \
    int deep;                        \
    int* arr;                                  \
    FILE *fp;                          \
    int current_size;\
    int current_index;\
    int current;                                \
    char *name;                                 \
    int *size;                                  \
    struct timeval start;                       \
    struct timeval finish;\
    struct pollfd poll;\
}


#include "coro_jmp.h"


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
    // quicksort(arr, 0, current_index - 1);
    *size = current_index;
    return arr;
    // yield the file
    // open for the write and truncate
}

void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

int partition(int arr[], int l, int h)
{
    int x = arr[h];
    int i = (l - 1);

    for (int j = l; j <= h - 1; j++) {
        if (arr[j] <= x) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[h]);
    return (i + 1);
}

void
quickSortIterative(int arr[], int l, int h)
{
    // Create an auxiliary stack
    int stack[h - l + 1];

    // initialize top of stack
    int top = -1;

    // push initial values of l and h to stack
    stack[++top] = l;
    stack[++top] = h;

    // Keep popping from stack while is not empty
    while (top >= 0) {
        // Pop h and l
        h = stack[top--];
        l = stack[top--];

        // Set pivot element at its correct position
        // in sorted array
        int p = partition(arr, l, h);

        // If there are elements on left side of pivot,
        // then push left side to stack
        if (p - 1 > l) {
            stack[++top] = l;
            stack[++top] = p - 1;
        }

        // If there are elements on right side of pivot,
        // then push right side to stack
        if (p + 1 < h) {
            stack[++top] = p + 1;
            stack[++top] = h;
        }
    }
}

int *
coro_sortfile()
{
    gettimeofday(&coro_this()->start, 0);
    coro_yield();
    coro_this()->fp = fopen(coro_this()->name, "r");
    // set polling
    coro_this()->poll.fd = fileno(coro_this()->fp);
    coro_this()->poll.events = POLLIN|POLLPRI;
    coro_yield();
    coro_this()->arr = malloc(INITIAL_SZ * sizeof(int));
    coro_yield();
    coro_this()->current_size = INITIAL_SZ;
    coro_yield();
    while (1) {
        if (poll(&coro_this()->poll, 1, 2000)) {
            coro_yield();
            if (fscanf(coro_this()->fp, "%d", &coro_this()->current) == 1) {
                if (coro_this()->current_size == coro_this()->current_index) {
                    coro_yield();
                    coro_this()->arr = realloc(coro_this()->arr, sizeof(int) * coro_this()->current_size * 2);
                    coro_yield();
                    coro_this()->current_size *= 2;
                    coro_yield();
                }
                coro_this()->arr[coro_this()->current_index++] = coro_this()->current;
            } else {
                break;
            }
        }
    }
    // no need for the read decsriptors
    fclose(coro_this()->fp);
    coro_yield();
    quickSortIterative(coro_this()->arr, 0, coro_this()->current_index - 1);
    coro_yield();
    *coro_this()->size = coro_this()->current_index;
    gettimeofday(&coro_this()->finish, 0);
    coro_finish();
    coro_wait_all();
    // yield the file
    // open for the write and truncate
}

long long timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_usec - t0.tv_usec) / 1000;
}
int
main(int argc, char **argv)
{
    struct timeval t0;
    struct timeval t1;
    // ini coroutines
    gettimeofday(&t0, 0);
    coro_count = argc - 1;
    coros = malloc(coro_count * sizeof(struct coro));
    for (int i = 0; i < coro_count; ++i) {
        if (coro_init(&coros[i]) != 0) {
            break;
        }
    }
    int *sorted[argc - 1];
    int sizes[argc - 1];
    int iterators[argc - 1];
    int overall = 0;
    for (int i = 1; i < argc; ++i) {
        iterators[i - 1] = 0;
        coros[i - 1].name = argv[i];
        coros[i - 1].size = &sizes[i - 1];
    }
    coro_call(coro_sortfile);
    for (int i = 1; i < argc; ++i) {
        sorted[i - 1] = coros[i - 1].arr;
        sizes[i - 1] = *coros[i - 1].size;
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
    for (int i = 0; i < argc - 1; ++i) {
        free(coros[i].arr);
    }
    free(coros);
    gettimeofday(&t1, 0);

    long microseconds = t1.tv_usec - t0.tv_usec + 1000000 * (t1.tv_sec - t0.tv_sec);
    printf("Overall working time %ld microseconds\n", microseconds);

    for (int i = 0; i < argc - 1; ++i) {
        microseconds = coros[i].finish.tv_usec - coros[i].start.tv_usec + 1000000 * (coros[i].finish.tv_sec - coros[i].start.tv_sec);
        printf("Coro #%d worked for %ld microseconds\n", i, microseconds);
    }
    return 0;
}