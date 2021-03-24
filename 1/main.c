#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <poll.h>
#include <unistd.h>

enum {
    INITIAL_SZ = 20000,
};

#define CORO_LOCAL_DATA struct {                \
    int deep;                        \
    int arr[INITIAL_SZ];                                  \
    FILE *fp;                          \
    int current_size;\
    int current_index;\
    int current;                                \
    char *name;                                 \
    int i;\
    int *size;                                  \
    struct timeval start;                       \
    struct timeval finish;\
    struct pollfd poll;\
}

#include "coro_jmp.h"
void merge(int arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 =  r - m;

    /* create temp arrays */
    int L[n1], R[n2];

    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1+ j];

    /* Merge the temp arrays back into arr[l..r]*/
    i = 0; // Initial index of first subarray
    j = 0; // Initial index of second subarray
    k = l; // Initial index of merged subarray
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
        {
            arr[k] = L[i];
            i++;
        }
        else
        {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    /* Copy the remaining elements of L[], if there
       are any */
    while (i < n1)
    {
        arr[k] = L[i];
        i++;
        k++;
    }

    /* Copy the remaining elements of R[], if there
       are any */
    while (j < n2)
    {
        arr[k] = R[j];
        j++;
        k++;
    }
}

/* l is for left index and r is right index of the
   sub-array of arr to be sorted */
void sort(int arr[], int l, int r)
{
    if (l < r)
    {
        // Same as (l+r)/2, but avoids overflow for
        // large l and h
        int m = l+(r-l)/2;

        // Sort first and second halves
        sort(arr, l, m);
        sort(arr, m+1, r);
        merge(arr, l, m, r);
    }
}

void
coro_sortfile()
{
    gettimeofday(&coro_this()->start, 0);
    coro_yield();
    coro_this()->fp = fopen(coro_this()->name, "r");
    // set polling
    coro_this()->poll.fd = fileno(coro_this()->fp);
    coro_this()->poll.events = POLLIN | POLLPRI;
    coro_yield();
    // coro_this()->arr = malloc(INITIAL_SZ * sizeof(int));
    coro_yield();
    coro_this()->current_size = INITIAL_SZ;
    coro_yield();
    while (1) {
        if (poll(&coro_this()->poll, 1, 2000)) {
            coro_yield();
            if (fscanf(coro_this()->fp, "%d", &coro_this()->current) == 1) {
//                if (coro_this()->current_size == coro_this()->current_index) {
//                    coro_yield();
//                    coro_this()->arr = realloc(coro_this()->arr, sizeof(int) * coro_this()->current_size * 2);
//                    coro_yield();
//                    coro_this()->current_size *= 2;
//                    coro_yield();
//                }
                coro_this()->arr[coro_this()->current_index++] = coro_this()->current;
                coro_yield();
            } else {
                break;
            }
        }
    }
    // no need for the read decsriptors
    coro_yield();
    sort(coro_this()->arr, 0, coro_this()->current_index - 1);
    coro_yield();
//    freopen(coro_this()->name, "w", coro_this()->fp);
//    coro_yield();
//    ftruncate(fileno(coro_this()->fp), 0);
//    coro_yield();
//    for (coro_this()->i = 0; coro_this()->i < coro_this()->current_index; ++coro_this()->i) {
//        coro_yield();
//        fprintf(coro_this()->fp, "%d ", coro_this()->arr[coro_this()->i]);
//    }
    fclose(coro_this()->fp);
    coro_yield();
    // free(coro_this()->arr);
    coro_yield();
    *coro_this()->size = coro_this()->current_index;
    coro_yield();
    gettimeofday(&coro_this()->finish, 0);
    coro_finish();
    coro_wait_all();
}

int
main(int argc, char **argv)
{
    struct timeval t0;
    struct timeval t1;
    // ini coroutines
    gettimeofday(&t0, 0);
    coro_count = argc - 1;
    // coros = calloc(coro_count, sizeof(struct coro));
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
        coros[i - 1].current_size = 0;
        coros[i - 1].current_index = 0;
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
    gettimeofday(&t1, 0);

    long microseconds = t1.tv_usec - t0.tv_usec + 1000000 * (t1.tv_sec - t0.tv_sec);
    printf("Overall working time %ld microseconds\n", microseconds);

    for (int i = 0; i < argc - 1; ++i) {
        // free(sorted[i - 1]);
        microseconds = coros[i].finish.tv_usec - coros[i].start.tv_usec +
                       1000000 * (coros[i].finish.tv_sec - coros[i].start.tv_sec);
        printf("Coro #%d worked for %ld microseconds\n", i, microseconds);
    }
    // free(coros);
    return 0;
}