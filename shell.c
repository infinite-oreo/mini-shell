/**
 * [INPUT]: 依赖 POSIX 标准库 — unistd / sys/wait / fcntl
 * [OUTPUT]: 可执行二进制 mysh，交互式 Unix shell
 * [POS]: 项目唯一源文件，实现 parse / builtin / redirect / pipeline 四大核心能力
 * [PROTOCOL]: 变更时更新此头部，然后检查 CLAUDE.md
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS  64
#define MAX_CMDS  16

typedef struct { char *in, *out; int append; } Redir;

/* ============================================================
 * 解析
 * ============================================================ */

static void parse(char *input, char **args) {
    int i = 0;
    for (char *tok = strtok(input, " "); tok && i < MAX_ARGS-1; tok = strtok(NULL, " "))
        args[i++] = tok;
    args[i] = NULL;
}

/* ============================================================
 * 重定向：从 args 中提取 < > >> 并将其从参数列表移除
 * ============================================================ */

static void extract_redir(char **args, Redir *r) {
    *r = (Redir){0};
    int i = 0, j = 0;
    while (args[i]) {
        if      (!strcmp(args[i], "<")  && args[i+1]) { r->in  = args[++i]; }
        else if (!strcmp(args[i], ">>") && args[i+1]) { r->out = args[++i]; r->append = 1; }
        else if (!strcmp(args[i], ">")  && args[i+1]) { r->out = args[++i]; }
        else args[j++] = args[i];
        i++;
    }
    args[j] = NULL;
}

static void apply_redir(const Redir *r) {
    if (r->in) {
        int fd = open(r->in, O_RDONLY);
        if (fd < 0) { perror(r->in); exit(1); }
        dup2(fd, STDIN_FILENO); close(fd);
    }
    if (r->out) {
        int flags = O_WRONLY | O_CREAT | (r->append ? O_APPEND : O_TRUNC);
        int fd = open(r->out, flags, 0644);
        if (fd < 0) { perror(r->out); exit(1); }
        dup2(fd, STDOUT_FILENO); close(fd);
    }
}

/* ============================================================
 * 内置命令
 * ============================================================ */

static int builtin(char **args) {
    if (!strcmp(args[0], "exit")) {
        printf("再见！\n");
        exit(0);
    }
    if (!strcmp(args[0], "cd")) {
        const char *dir = args[1] ? args[1] : getenv("HOME");
        if (chdir(dir))
            fprintf(stderr, "mysh: cd: 目录不存在: %s\n", dir);
        return 1;
    }
    if (!strcmp(args[0], "pwd")) {
        char cwd[MAX_INPUT];
        if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        return 1;
    }
    return 0;
}

/* ============================================================
 * 执行
 * ============================================================ */

/* 在子进程中调用：提取重定向后 exec */
static void run_cmd(char **args) {
    Redir r;
    extract_redir(args, &r);
    apply_redir(&r);
    execvp(args[0], args);
    fprintf(stderr, "mysh: 命令未找到: %s\n", args[0]);
    exit(1);
}

static void execute(char **args) {
    if (!fork()) run_cmd(args);
    wait(NULL);
}

/* 按 | 分割参数段，构造任意长度的管道链 */
static void execute_pipeline(char **args) {
    char **cmds[MAX_CMDS];
    int    ncmds = 0;
    cmds[ncmds++] = args;
    for (int i = 0; args[i]; i++)
        if (!strcmp(args[i], "|")) { args[i] = NULL; cmds[ncmds++] = args + i + 1; }

    int fds[MAX_CMDS-1][2];
    for (int i = 0; i < ncmds-1; i++) pipe(fds[i]);

    for (int i = 0; i < ncmds; i++) {
        if (!fork()) {
            if (i > 0)       dup2(fds[i-1][0], STDIN_FILENO);
            if (i < ncmds-1) dup2(fds[i][1],   STDOUT_FILENO);
            for (int j = 0; j < ncmds-1; j++) { close(fds[j][0]); close(fds[j][1]); }
            run_cmd(cmds[i]);
        }
    }
    for (int i = 0; i < ncmds-1; i++) { close(fds[i][0]); close(fds[i][1]); }
    for (int i = 0; i < ncmds; i++) wait(NULL);
}

/* ============================================================
 * 提示符：显示当前目录，$HOME 前缀替换为 ~
 * ============================================================ */

static void print_prompt(void) {
    char cwd[MAX_INPUT], buf[MAX_INPUT];
    if (!getcwd(cwd, sizeof(cwd))) { printf("mysh> "); return; }

    const char *home    = getenv("HOME");
    const char *display = cwd;
    if (home && !strncmp(cwd, home, strlen(home))) {
        snprintf(buf, sizeof(buf), "~%s", cwd + strlen(home));
        display = buf;
    }
    printf("mysh:%s> ", display);
}

static int has_pipe(char **args) {
    for (int i = 0; args[i]; i++)
        if (!strcmp(args[i], "|")) return 1;
    return 0;
}

/* ============================================================
 * 主循环
 * ============================================================ */

int main(void) {
    char  input[MAX_INPUT];
    char *args[MAX_ARGS];

    while (1) {
        print_prompt();
        fflush(stdout);

        if (!fgets(input, MAX_INPUT, stdin)) break;
        input[strcspn(input, "\n")] = '\0';
        if (!input[0]) continue;

        parse(input, args);

        if (has_pipe(args))
            execute_pipeline(args);
        else if (!builtin(args))
            execute(args);
    }
    return 0;
}
