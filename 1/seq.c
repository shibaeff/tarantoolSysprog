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

enum {
    INITIAL_SZ = 1
};

/*
 * read and sort the single file
 */
int*
sortfile(char *name, int* size)
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
    // yield the file
    // open for the write and truncate
}

int
main(int argc, char **argv)
{
    int fd = open("test1.txt", O_RDONLY);

    int* sorted[argc - 1];
    int sizes[argc - 1];
    int iterators[argc - 1];
    int overall = 0;
    for (int i = 1; i < argc; ++i) {
        iterators[i - 1] = 0;
        sorted [i - 1] = sortfile(argv[i], &sizes[i - 1]);
        overall += sizes[i - 1];
    }

    FILE* out = fopen("output.txt", "w");
    // suboptimal k-way merge
    for (int i = 0; i < overall; ++i) {
        int value = INT_MAX;
        int index = -1;
        for (int k = 0; k < argc - 1; ++k) {
            if (iterators[i] < sizes[i] && sorted[k][iterators[i]] < value) {
                index = k;
                value = iterators[i];
            }
        }
        fprintf(out, "%d ", sorted[index][iterators[i]++]);
    }
    fclose(out);
//    for (int i = 1; i < argc; ++i) {
//        close(fds[i - 1]);
//    }
    return 0;
}