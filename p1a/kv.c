#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define MAX_ELEMENT_COUNT 100003 // a prime for better hashing
#define MAX_LAZY_DELETE 10000
#define MAX_STRING_LENGTH 100

int lazyDelCount = 0;

void initDB();

void exportDB();

void importDB();

int split(char dst[2][MAX_STRING_LENGTH], char str[MAX_STRING_LENGTH]);

// an open addressing HashMap
static int db_key[MAX_ELEMENT_COUNT];
static char db_val[MAX_ELEMENT_COUNT][MAX_STRING_LENGTH];

// find pos for key x
int find(int x) {
    int N = MAX_ELEMENT_COUNT;
    int t = (x % N + N) % N;
    while (db_key[t] != INT_MAX && db_key[t] != x) {
        t++;
        if (t == N) t = 0;
    }
    return t;
}

// add key-val to hashmap
void put(int key, char *val) {
    int t = find(key);
    db_key[t] = key;
    strcpy(db_val[t], val);
}

int get(int key) {
    int t = find(key);
    if (db_key[t] == INT_MAX) return -1;
    if (strcmp(db_val[t], ",") == 0) return -1;
    return t;
}

// lazy delete when too many lazy deleted label ","
void lazyDelete() {
    exportDB();
    initDB();
    importDB();
}

void del(int key) {
    int t = find(key);
    if (db_key[t] == INT_MAX || strcmp(db_val[t], ",") == 0) printf("%d not found\n", key);
    else {
        strcpy(db_val[t], ",");
        lazyDelCount++;
        if (lazyDelCount > MAX_LAZY_DELETE) lazyDelete();
    }
}

void clr() {
    initDB();
}

void printAll() {
    for (int i = 0; i < MAX_ELEMENT_COUNT; ++i) {
        if (db_key[i] != INT_MAX && strcmp(db_val[i], ",") != 0) {
            printf("%d,%s\n", db_key[i], db_val[i]);
        }
    }
}

int isNum(char *str) {
    return (strspn(str, "0123456789") == strlen(str));
}

void importDB() {
    FILE *fp;
    if ((fp = fopen("database.txt", "a+")) == NULL) {
        printf("Unexpected Error: Fail to open the database file!\n");
        exit(0);
    }

    char str[MAX_STRING_LENGTH];
    while (~fscanf(fp, "%s", str)) {
        char dst[2][MAX_STRING_LENGTH];
        split(dst, str);
        put(atoi(dst[0]), dst[1]);
    }

    if (fclose(fp) != 0) {
        printf("Unexpected Error: Fail to close the database file!\n");
        exit(0);
    }
}

int split(char dst[][MAX_STRING_LENGTH], char *str) {
    int n = 0;
    char *result = NULL;
    result = strtok(str, ",");
    while (result != NULL) {
        strcpy(dst[n++], result);
        result = strtok(NULL, ",");
    }
    return n;
}

void exportDB() {
    FILE *fp;
    if ((fp = fopen("database.txt", "w")) == NULL) {
        printf("Unexpected Error: Fail to open the database file!\n");
        exit(0);
    }

    for (int i = 0; i < MAX_ELEMENT_COUNT; ++i) {
        if (db_key[i] != INT_MAX && strcmp(db_val[i], ",") != 0) {
            fprintf(fp, "%d,%s\n", db_key[i], db_val[i]);
        }
    }

    if (fclose(fp) != 0) {
        printf("Unexpected Error: Fail to close the database file!\n");
        exit(0);
    }
}

void initDB() {
    for (int i = 0; i < MAX_ELEMENT_COUNT; ++i) {
        db_key[i] = INT_MAX;
    }
}

void handleInputs(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        char dst[3][MAX_STRING_LENGTH];
        int cnt = split(dst, argv[i]);
        if (strcmp(dst[0], "p") == 0) {
            if (cnt != 3 || !isNum(dst[1])) printf("bad command\n");
            else put(atoi(dst[1]), dst[2]);
        } else if (strcmp(dst[0], "g") == 0) {
            if (cnt != 2 || !isNum(dst[1])) printf("bad command\n");
            else {
                int t = get(atoi(dst[1]));
                if (t == -1) printf("%s not found\n", dst[1]);
                else printf("%s,%s\n", dst[1], db_val[t]);
            }
        } else if (strcmp(dst[0], "d") == 0) {
            if (cnt != 2 || !isNum(dst[1])) printf("bad command\n");
            else del(atoi(dst[1]));
        } else if (strcmp(dst[0], "c") == 0) {
            if (cnt != 1) printf("bad command\n");
            else clr();
        } else if (strcmp(dst[0], "a") == 0) {
            if (cnt != 1) printf("bad command\n");
            else printAll();
        } else {
            printf("bad command\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) return 0;
    initDB();
    importDB();
    handleInputs(argc, argv);
    exportDB();
    return 0;
}
