# C-Shell

This project is a shell-like program written in C that mimics some functionalities of a standard shell interface. It displays a custom command prompt, shell>, and executes commands entered by the user. The program supports both built-in commands and external system commands, along with features for managing processes and command chaining.

## Features

- <strong> Built-in Commands: </strong> The shell supports four built-in commands.
  - <strong> cd <directory>: </strong> Changes the current working directory to the specified path. It supports both relative and absolute paths and changes to the $HOME directory if no argument is provided. It updates the pwd environment variable and uses the chdir() function.
  - <strong> pwd: </strong> Prints the current working directory by calling the getcwd() function.
  - <strong> history: </strong> Prints the last 10 commands entered in the current session. It maintains a FIFO data structure with a capacity of 10 for this purpose.
  - <strong> exit: </strong> Terminates the shell process by calling exit(0).
- <strong> System Commands: </strong> The shell can execute other commands by treating them as system commands. It creates a child process using fork(), and the child process executes the command using execvp().
- <strong> Background Processes: </strong> The program supports running system commands in the background by appending an ampersand (&) to the command line input. The shell does not wait for background processes to complete and instead displays the process ID before prompting the user for the next command.
- <strong> Piping: </strong> The program can handle a single pipe operator (|). It redirects the standard output (stdout) of the command on the left to the standard input (stdin) of the command on the right.
- <strong> Logical AND (&&): </strong> The shell supports the && operator to conditionally chain two processes. The command on the right is only executed if the command on the left is successful.

## How to Compile and Run

To compile the source code, use a C compiler such as gcc.

```
# Compilation command
gcc -o my_shell your_program.c

# To run the program
./my_shell

```

Once executed, the program will display the shell> prompt, ready to accept user commands.

## Assumptions and Constraints

- Command line input will be at most 100 characters and contain at most 10 arguments.
- Arguments are delimited by a space character.
- At most one of &, |, or && will be used in a single input line.
