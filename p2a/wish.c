#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>


#define MAXL 1000


int num = 1;
int fileW = 1;
int loop_num = 1;
int loop_var = 0;
char *fp = NULL;
char *path[MAXL];
char *pars[MAXL];

void error(int x) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int sep_str(char *args[], char *line, char *sep) {
    int cnt = 0;
    char *l = strdup(line);
    for (args[cnt] = strsep(&l, sep); args[cnt] != NULL; args[cnt] = strsep(&l, sep)) {
        if (strlen(args[cnt]) == 0) continue;
        cnt++;
    }
    return 0;
}

#define DONE 1
#define PASS 0

int get_dir(char *p[], char *pars[], char *dir) {
    if (pars == NULL || pars[0] == NULL) return 1;
    int found = 0;
    if (p[0] == NULL) {
        error(1);
        return DONE;
    }
    for (int i = 0; i < num; i++) {
        char tmp[MAXL];
        strcpy(tmp, p[i]);
        int l = strlen(tmp);
        tmp[l] = '/';
        tmp[l + 1] = '\0';
        strcat(tmp, pars[0]);
        if (access(tmp, X_OK) == 0) {
            strcpy(dir, tmp);
            found = 1;
            break;
        }
    }

    if (!found) {
        error(2);
        return DONE;
    } else if (dir == NULL) {
        error(4);
        return DONE;
    } else if (p[0] == NULL) {
        error(3);
        return DONE;
    } else {
        return PASS;
    }
}


int execute(char *args[], int fileW, char *dir) {
//    int childpid;
//    int childStatus;
    int fc = fork();
    if (fc == 0) {
        if (fileW == 0) {
            int fd_out = open(fp, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
            if (fd_out > 0) dup2(fd_out, STDOUT_FILENO);
        }
        if (args[0] == NULL)
            return -1;

        execv(dir, args);
    } else {
        wait(NULL);
    }
    return 0;
}

int redirect(char *ret, char *line, char *args[]) {
    ret[0] = '\0';
    ret = ret + 1;
    char *retargs[10];
    sep_str(args, line, "\t\n ");
    sep_str(retargs, ret, "\t\n ");
    fp = retargs[0];
    if (fp == NULL) {
        error(11);
        return DONE;
    }
    if (args[0] == NULL) {
        error(12);
        return DONE;
    }
    if (retargs[1]) {
        error(13);
        return DONE;
    }
    char dir[MAXL];
    get_dir(path, args, dir);
    execute(args, 0, dir);
    return PASS;
}


char *toS(int n, char *str) {
    int i = 0;
    if (n < 0) {
        n = -n;
        str[i++] = '-';
    }
    do {
        str[i++] = n % 10 + 48;
        n /= 10;
    } while (n);
    str[i] = '\0';
    int j = 0;
    if (str[0] == '-') {
        j = 1;
        ++i;
    }
    for (; j < i / 2; j++) {
        str[j] = str[j] + str[i - 1 - j];
        str[i - 1 - j] = str[j] - str[i - 1 - j];
        str[j] = str[j] - str[i - 1 - j];
    }
    return str;
}


int buildin_handler(char *path[], char *args[]) {
    loop_num = 1;
    loop_var = -1;
    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) error(5);
        else exit(0);
        return -1;
    } else if (strcmp(args[0], "cd") == 0) {
        if ((!args[1]) || args[2]) {
            error(6);
        } else if (chdir(args[1]) == -1) error(7);
        return -1;
    } else if (strcmp(args[0], "path") == 0) {
        num = 0;
        for (int i = 0; i < MAXL; i++) path[i] = NULL;
        if (args[1] == NULL) {
            path[0] = NULL;
        }
        int i = 0;
        for (i = 0; args[i + 1] != NULL; i++) {
            path[i] = args[i + 1];
        }
        num = i;
        return -1;
    } else if (strcmp(args[0], "loop") == 0) {
        if (args[1] == NULL) {
            error(8);
            return -1;
        }
        loop_num = atoi(args[1]);
        if (loop_num == 0 && strcmp(args[1], "0") != 0) {
            error(9);
            loop_num = 1;
            return -1;
        }
        if (loop_num < 0) {
            error(10);
            loop_num = 1;
            return -1;
        }
        int i;
        for (i = 2; args[i] != NULL; i++) {
            args[i - 2] = strdup(args[i]);
        }
        args[i - 2] = NULL;
        for (i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "$loop") == 0) loop_var = i;
        }
    }
    return 0;
}

int parse(char *args[], FILE *fp) {
    char *line = NULL;
    size_t size = 0;
    if (getline(&line, &size, fp) == -1) {
        //  exit(1); // todo
        return 1;
    }
    if ((strcmp(line, "\n") == 0) || (strcmp(line, "") == 0))
        return -1;
    line[strlen(line) - 1] = '\0';
    if (line[0] == EOF) return 1;

    char *ret = strchr(line, '>');
    if (ret) {
        redirect(ret, line, args);
        return -1;
    }
    sep_str(args, line, "\t\n ");
    if (args[0] == NULL)
        return -1;
    return 0;
}

int main(int argc, char *argv[]) {
    char dir[MAXL];
    path[0] = "/bin";
    if (argc < 1 || argc > 2) {
        error(14);
        exit(1);
    }
    if (argc == 2) {
        FILE *fp;
        if (!(fp = fopen(argv[1], "r+"))) {
            error(15);
            exit(1);
        }
        while (1) {
            int read = parse(pars, fp);
            if (read == -1)
                continue;
            if (read == 1)
                break;
            if (!pars[0])
                break;

            if (buildin_handler(path, pars) == -1)
                continue;

            for (int i = 0; i < loop_num; i++) {
                char ns[MAXL] = {0};
                if (loop_var != -1) pars[loop_var] = strdup(toS(i + 1, ns));
                if (get_dir(path, pars, dir) == 1)
                    continue;
                execute(pars, fileW, dir);
            }

            loop_num = 1;
            loop_var = -1;
        }
        return 0;
    }

    while (1) {
        printf("wish> ");
        int status = parse(pars, stdin);
        if (status == -1) continue;
        if (status == 1) break;

        if (buildin_handler(path, pars) == -1)
            continue;

        for (int i = 0; i < loop_num; i++) {
            char ns[MAXL] = {0};
            if (loop_var != -1) pars[loop_var] = strdup(toS(i + 1, ns));
            if (get_dir(path, pars, dir) == 1)
                continue;
            execute(pars, fileW, dir);
        }

        loop_num = 1;
        loop_var = -1;
    }

    return 0;
}
