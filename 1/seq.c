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
    INITIAL_SZ = 16
};

/*
 * sort a single file point by the descriptor
 */
void
sortfile(int fd)
{
    char *arr = malloc(INITIAL_SZ * sizeof(int));
    int current_size = INITIAL_SZ;
    int current_index = 0;
    int current;
    FILE *fp = fdopen(fd);
    while (fscanf(fd, "%d", &current) == 1) {
        if (current_size == current_index) {
            arr = realloc(arr, sizeof(int) * current_size * 2);
            current_size *= 2;
        }
        arr[current_index++] = current;
    }
    // no need for the read decsriptors
    close(fd);
    fclose(fp);
    quicksort(arr, 0, current_index);
    // yield the file
    // open for the write and truncate
    fd = open()
    for (int i = 0; i < current_index; ++i) {
        fprintf(fp, "%d ", arr[i]);
    };
    free(arr);
}

int
main(int argc, char **argv)
{
    int fds[argc - 1]; // array of file descriptors
    for (int i = 1; i < argc; ++i) {
        fds[i - 1] = open(argv[i], O_RDONLY, 0600); // first open in the read-only mode
        check(fds[i - 1] == -1, "failed to open file %s", argv[i]);
    }
//    for (int i = 1; i < argc; ++i) {
//        close(fds[i - 1]);
//    }
    return 0;
}