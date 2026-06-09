# Mini Shell

A minimal Unix shell implemented in C, ~160 lines.

## Features

- Execute external commands (`ls`, `pwd`, `echo`, etc.)
- Built-in `cd`, `pwd`, `exit`
- **Multi-stage pipelines** — `ls | grep .c | wc -l`
- **I/O redirection** — `>`, `>>`, `<`
- **Smart prompt** — shows current directory with `~` shorthand

## Build & Run

```bash
gcc shell.c -o mysh
./mysh
```

## Examples

```
mysh:~> ls -la
mysh:~> cd Documents
mysh:~/Documents> ls | grep .c | wc -l
mysh:~/Documents> echo hello > out.txt
mysh:~/Documents> cat >> out.txt
mysh:~/Documents> cat < out.txt
mysh:~/Documents> exit
```

## What I learned

- How Unix processes work (`fork`, `execvp`, `wait`)
- File descriptors and I/O redirection (`dup2`, `pipe`)
- How a real shell chains N commands through N-1 pipes
- How `>` / `>>` / `<` are implemented via `open` + `dup2`
