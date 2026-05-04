# Mini Shell

A minimal Unix shell implemented in C.

## Features

- Execute external commands (`ls`, `pwd`, `echo`, etc.)
- Built-in `cd` command
- Built-in `exit` command
- Pipeline support (`ls | grep .c`)

## Build & Run

```bash
gcc shell.c -o mysh
./mysh
```

## Example
mysh> ls -la
mysh> cd Documents
mysh> ls | grep .c
mysh> exit

## What I learned

- How Unix processes work (`fork`, `execvp`, `wait`)
- File descriptors and I/O redirection (`dup2`, `pipe`)
- How a real shell executes commands under the hood