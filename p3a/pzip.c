#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>

typedef struct {
    int count;
    char c;
} __attribute__((packed)) charc;

typedef struct {
    int begin;
    int term;
} targ;

typedef struct {
    charc *data;
    int datasize;
} tres;

char *f;

void *parser(void *arg) {
    targ *args = (targ *) arg;
    tres *res = malloc(sizeof(tres));
    int datasize = 0, counter = 0, start = args->begin, end = args->term;
    char curr;
    res->data = malloc(5 * (end - start + 1));
    while (f[start] == '\0')
        start++;
    curr = f[start];
    while (start <= end) {
        if (f[start] == '\0') {
            start++;
        } else if (f[start] == curr) {
            start++;
            counter++;
        } else {
            charc temp = {counter, curr};
            res->data[datasize++] = temp;
            counter = 0;
            curr = f[start];
        }
    }
    charc temp = {counter, curr};
    res->data[datasize++] = temp;
    res->datasize = datasize;
    return (void *) res;
}

// Cited from: https://stackoverflow.com/questions/20460670/reading-a-file-to-string-with-mmap
int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("pzip: file1 [file2 ...]\n");
        return 1;
    }
    int size, portion;
    struct stat s;
    int threads = get_nprocs() * 2 + 1, curcharc = 0, curmax = 16;
    charc *collcharc = malloc(curmax * 5);
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1)
            continue;
        fstat(fd, &s);
        size = s.st_size;
        f = (char *) mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (f == MAP_FAILED)
            continue;
        if (size < 33) {
            pthread_t p;
            tres *res;
            targ args = {0, size - 1};
            pthread_create(&p, NULL, parser, &args);
            pthread_join(p, (void **) &res);
            int start = 0;
            if (collcharc != 0 && res->data[0].c == collcharc[curcharc - 1].c) {
                collcharc[curcharc - 1].count += res->data[0].count;
                start = 1;
            }
            while (start < res->datasize) {
                collcharc[curcharc++] = res->data[start++];
                if (curcharc == curmax) {
                    collcharc = realloc(collcharc, curmax * 2 * 5);
                    curmax *= 2;
                }
            }
            free(res->data);
            free(res);
        } else {
            portion = size / threads;
            pthread_t *pths = malloc(threads * sizeof(pthread_t));
            targ **args = malloc(threads * sizeof(targ *));
            tres **res = malloc(threads * sizeof(tres *));
            for (int j = 0; j < threads; j++) {
                args[j] = malloc(sizeof(targ));
                args[j]->begin = j * portion;
                if (j == threads - 1) {
                    args[j]->term = size - 1;
                } else {
                    args[j]->term = (j + 1) * portion - 1;
                }
                pthread_create(&pths[j], NULL, parser, args[j]);
            }
            for (int j = 0; j < threads; j++) {
                pthread_join(pths[j], (void **) &res[j]);
                int start = 0;
                if (collcharc != 0 && res[j]->data[0].c == collcharc[curcharc - 1].c) {
                    collcharc[curcharc - 1].count += res[j]->data[0].count;
                    start = 1;
                }
                while (start < res[j]->datasize) {
                    collcharc[curcharc++] = res[j]->data[start++];
                    if (curcharc == curmax) {
                        collcharc = realloc(collcharc, curmax * 2 * 5);
                        curmax *= 2;
                    }
                }
                free(args[j]);
                free(res[j]->data);
                free(res[j]);
            }
            free(pths);
            free(args);
            free(res);
        }
        close(fd);
    }
    fwrite(collcharc, 5, curcharc, stdout);
    free(collcharc);
    return 0;
}