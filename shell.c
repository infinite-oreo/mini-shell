#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

void parse(char *input, char **args) {
    int i = 0;
    char *token = strtok(input, " ");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

int builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        printf("再见！\n");
        exit(0);
    }
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            chdir(getenv("HOME"));
        } else {
            if (chdir(args[1]) != 0) {
                printf("mysh: cd: 目录不存在: %s\n", args[1]);
            }
        }
        return 1;
    }
    return 0;
}

void execute(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            printf("mysh: 命令未找到: %s\n", args[0]);
            exit(1);
        }
    } else {
        wait(NULL);
    }
}

// 检查输入里有没有管道符，有就返回它的位置，没有返回-1
int find_pipe(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            return i;
        }
    }
    return -1;
}

void execute_pipe(char **args, int pipe_pos) {
    // 把参数分成管道左边和右边
    char **left = args;
    char **right = args + pipe_pos + 1;
    args[pipe_pos] = NULL;  // 在管道位置截断

    int fd[2];
    pipe(fd);  // 创建管道，fd[0]是读端，fd[1]是写端

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // 左边的命令：把输出写到管道
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);  // 把标准输出重定向到管道写端
        close(fd[1]);
        if (execvp(left[0], left) == -1) {
            printf("mysh: 命令未找到: %s\n", left[0]);
            exit(1);
        }
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // 右边的命令：从管道读输入
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);  // 把标准输入重定向到管道读端
        close(fd[0]);
        if (execvp(right[0], right) == -1) {
            printf("mysh: 命令未找到: %s\n", right[0]);
            exit(1);
        }
    }

    // 父进程关闭管道，等待两个子进程结束
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    while (1) {
        printf("mysh> ");
        fflush(stdout);

        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) continue;

        parse(input, args);

        int pipe_pos = find_pipe(args);

        if (pipe_pos != -1) {
            execute_pipe(args, pipe_pos);
        } else if (!builtin(args)) {
            execute(args);
        }
    }

    return 0;
}