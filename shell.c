#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>

char buffer[100]; // Holds the full user input line entered into the shell before parsing.
char args[10][100]; // Stores the left side of the command if a pipe (`|`) or AND (`&&`) is detected.
char args_right[10][100]; // Stores the right side of the command if a pipe (`|`) or AND (`&&`) is detected.
char history[10][1024]; // Stores the last 10 full commands entered by the user.
int history_size = 0; // Keeps track of how many commands are currently stored in the `history` array.
bool has_pipe = false; // Set to true if the user input contains a pipe symbol (`|`).
bool has_and = false; // Set to true if the user input contains an AND operator (`&&`).
char *pipe_char = "|"; // A string literal representing the pipe symbol.
char *and_char = "&&"; // A string literal representing the logical AND operator

// Removes newline from user input.
void trim_newline(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

// Parses the input command and fills args and args_right.
void parse_command(char buffer[])
{
    // Allocate memory for a copy of the buffer (input command).
    char *line = malloc(strlen(buffer) + 1); // +1 space for null terminator '\0'
    // If malloc fails, print error and exit.
    if (line == NULL)
    {
        perror("malloc failed");
        exit(1);
    }

    // Copy the input buffer to the newly allocated line.
    strcpy(line, buffer);

    // Check if the last character in the input is '&' (used for background execution).
    if (buffer[strlen(buffer) - 1] == '&')
    {
        // If so, remove it by replacing it with '\0'.
        line[strlen(line) - 1] = '\0';
    }

    // Tokenize the line using space as a delimiter.
    char *tkn = strtok(line, " ");

    // Indexes to keep track of positions in args and args_right.
    int i = 0, j = 0;

    // Continue tokenizing until all tokens are processed.
    while (tkn != NULL)
    {
        // Check if the current token is a pipe symbol "|".
        if (strcmp(pipe_char, tkn) == 0)
        {
            has_pipe = true;
        }
        // Check if the current token is the AND operator "&&"
        else if (strcmp(and_char, tkn) == 0)
        {
            has_and = true;
        }
        // If either a pipe or AND operator was previously encountered,
        // remaining tokens belong to the right side of the operator.
        else if (has_pipe == true || has_and == true)
        {
            strcpy(args_right[j++], tkn); // Store in args_right
        }
        else
        {
            strcpy(args[i++], tkn); // Otherwise, store in args (left side of command)
        }

        // Move to the next token
        tkn = strtok(NULL, " ");
    }
    // printf("buffer in parse after: %s\n", buffer);
}

// Adds a command to the command history.
// If history is full (10 entries), removes the oldest command and appends the new one at the end.
void add_history(char *buffer)
{
    // If history is not full, just add the new command to the next slot.
    if (history_size < 10)
    {
        strcpy(history[history_size], buffer); // Copy the command into the history array.
        history_size++; // Increment the number of stored history entries.
    }
    else
    {
        // If history is full (10 entries), shift all entries one position to the left.
        for (int i = 0; i < 9; i++)
        {
            strcpy(history[i], history[i + 1]); // Overwrite entry i with entry i+1.
        }
        strcpy(history[9], buffer); // Add the new command as the last entry.
    }
}

// Prints the command history to the console.
void print_history()
{
    // Loop through all 10 possible history slots.
    for (int i = 0; i < 10; i++)
    {
        // Stop printing if it hits an empty entry.
        if (strlen(history[i]) == 0)
        {
            break;
        }
        // Print the history entry.
        printf("%d: %s\n", i + 1, history[i]);
    }
}

// Executes a system command, either from the left or right side of a pipe or AND operator.
// If `is_right` is true, it executes the command from `args_right`, otherwise from `args`.
// It also handles background execution if the command ends with '&'.
int process_system_command(char *buffer, bool is_right)
{
    int pid = fork(); // Create a new process

    // Child process block
    if (pid == 0)
    {
        // If the command ends with '&', redirect output to /dev/null.
        // This prevents background processes from printing to terminal.
        if (buffer[strlen(buffer) - 1] == '&')
        {
            int null_fd = open("/dev/null", O_WRONLY); // Open /dev/null for writing
            dup2(null_fd, STDOUT_FILENO); // Redirect stdout to /dev/null
            dup2(null_fd, STDERR_FILENO); // Redirect stderr to /dev/null
            close(null_fd); // Close the file descriptor
        }

        // Array to hold command and arguments, plus NULL terminator
        char *argv[11];
        int i = 0;

        // Prepare argv from either args or args_right depending on the flag.
        if (!is_right)
        {
            // Copy arguments from `args` to argv.
            while (i < 10 && args[i][0] != '\0')
            {
                argv[i] = args[i];
                i++;
            }
        }
        else
        {
            // Copy arguments from `args_right` to argv.
            while (i < 10 && args_right[i][0] != '\0')
            {
                argv[i] = args_right[i];
                i++;
            }
        }

        argv[i] = NULL; // NULL terminate the array (required by execvp).

        // Try to execute the command.
        int status_code = execvp(argv[0], argv);

        // // If execvp fails, terminate child process with error code 127.
        // _exit(127);

        if (status_code != 0) {
            printf("Process did not terminate correctly\n");
            exit(1);
        }
    }

    // Parent process block
    else if (pid > 0)
    {
        int status;

        // If the command is not a background command (doesn't end with '&').
        if (buffer[strlen(buffer) - 1] != '&')
        {
            // Wait for the child process to complete
            waitpid(pid, &status, 0);

            // Return the child's exit status if it exited normally.
            if (WIFEXITED(status))
            {
                return WEXITSTATUS(status); // Return child exit status
            }
            else
            {
                return 1; // Child did not exit normally
            }
        }
        else
        {
            // For background processes, don't wait just print the PID.
            printf("Started background process with PID: %d\n", pid);
            return 1; // Return success status to parent shell.
        }
    }

    // fork failed
    else
    {
        perror("fork failed");
        return 1; // Indicate failure
    }
}

// It handles built-in commands.
void execute_command(char *buffer)
{
    int status;

    // CASE 1: No pipes, no && — just a simple command
    if (has_pipe == false && has_and == false)
    {
        // "cd" command: change directory
        if (strcmp(args[0], "cd") == 0)
        {
            char path[100];
            if (strlen(args[1]) > 0)
            {
                // If given use the given path
                strcpy(path, args[1]);
            }
            else
            {
                char *home = getenv("HOME");
                strcpy(path, home); // If no path given, go to HOME
            }

            int res = chdir(path); // Change directory to path

            // If chdir fails exit.
            if (res == -1)
            {
                printf("Error at cd %s\n", args[1]);
                exit(-1);
            }

            setenv("PWD", path, 1); // Update PWD environment variable
        }
        // "pwd" command: print current working directory
        else if (strcmp(args[0], "pwd") == 0)
        {
            char pwd2[1024];
            getcwd(pwd2, 1024);
            printf("%s", pwd2);
        }
        // "history" command: show command history
        else if (strcmp(args[0], "history") == 0)
        {
            print_history();
        }
        // "exit" command: quit the shell
        else if (strcmp(args[0], "exit") == 0)
        {
            printf("Exitting from the program\n");
            exit(0);
        }
        // System command (ls, echo, etc.)
        else
        {
            process_system_command(buffer, false);
        }
    }
    // CASE 2: Command with &&
    else if (has_pipe == false && has_and == true)
    {
        // Fork first child for the left-side command.
        pid_t pid1 = fork();
        // left command
        if (pid1 == 0)
        {
            // "cd" command: change directory
            if (strcmp(args[0], "cd") == 0)
            {
                char path[100];
                if (strlen(args[1]) > 0)
                {
                    // If given use the given path
                    strcpy(path, args[1]);
                }
                else
                {
                    char *home = getenv("HOME");
                    strcpy(path, home); // If no path given, go to HOME
                }

                int res = chdir(path); // Change directory to path

                // If chdir fails exit.
                if (res == -1)
                {
                    printf("Error at cd %s\n", args[1]);
                    exit(-1);
                }

                setenv("PWD", path, 1); // Update PWD environment variable
                exit(0);
            }
            // "pwd" command: print current working directory
            else if (strcmp(args[0], "pwd") == 0)
            {
                char pwd2[1024];
                getcwd(pwd2, 1024);
                exit(0);
            }
            // "history" command: show command history
            else if (strcmp(args[0], "history") == 0)
            {
                print_history();
                exit(0);
            }
            // System command: run and return status (ls, echo, etc.)
            else
            {
                status = process_system_command(buffer, false);
                exit(status);
            }
        }

        // Parent waits for left command to finish
        int status1;
        waitpid(pid1, &status1, 0);
        int exit_code = WEXITSTATUS(status1);
        int sig = WTERMSIG(status1);

        // If it exited successfully, execute the right command
        if ((WIFEXITED(status1) && WEXITSTATUS(status1) == 0))
        {
            pid_t pid2 = fork();
            // Right-side command after && - read from pipe
            if (pid2 == 0)
            {
                // "cd" command: change directory
                if (strcmp(args_right[0], "cd") == 0)
                {
                    char path[100];
                    if (strlen(args_right[1]) > 0)
                    {
                        // If given use the given path
                        strcpy(path, args_right[1]);
                    }
                    else
                    {
                        char *home = getenv("HOME");
                        strcpy(path, home); // If no path given, go to HOME
                    }

                    int res = chdir(path); // Change directory to path

                    // If chdir fails exit.
                    if (res == -1)
                    {
                        printf("Error at cd %s\n", args_right[1]);
                        exit(-1);
                    }

                    setenv("PWD", path, 1); // Update PWD environment variable
                    exit(0);
                }
                // "pwd" command: print current working directory
                else if (strcmp(args_right[0], "pwd") == 0)
                {
                    char pwd2[1024];
                    getcwd(pwd2, 1024);
                    exit(0);
                }
                // "history" command: show command history
                else if (strcmp(args_right[0], "history") == 0)
                {
                    print_history();
                    exit(0);
                }
                else
                {
                    // Run system command on the right side
                    int status = process_system_command(buffer, true);
                    exit(status);
                }
            }

            // Parent waits for right-side command
            waitpid(pid2, NULL, 0);
        }
    }

    // CASE 3: Command with pipe (|)
    else if (has_pipe == true && has_and == false)
    {
        int pipefd[2];
        pipe(pipefd); // Create pipe

        pid_t pid1 = fork(); // First child for left command
        // left command
        if (pid1 == 0)
        {
            // Redirect stdout to pipe's write end
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]); // Close unused read end
            close(pipefd[1]);

            // "cd" command: change directory
            if (strcmp(args[0], "cd") == 0)
            {
                char path[100];
                if (strlen(args[1]) > 0)
                {
                    // If given use the given path
                    strcpy(path, args[1]);
                }
                else
                {
                    char *home = getenv("HOME");
                    strcpy(path, home); // If no path given, go to HOME
                }

                int res = chdir(path); // Change directory to path

                // If chdir fails exit.
                if (res == -1)
                {
                    printf("Error at cd %s\n", args[1]);
                    exit(-1);
                }

                setenv("PWD", path, 1); // Update PWD environment variable
                exit(0);
            }
            // "pwd" command: print current working directory
            else if (strcmp(args[0], "pwd") == 0)
            {
                char pwd2[1024];
                getcwd(pwd2, 1024);
                exit(0);
            }
            // "history" command: show command history
            else if (strcmp(args[0], "history") == 0)
            {
                print_history();
                exit(0);
            }
            else
            {
                // System command on left side
                process_system_command(buffer, false);
                exit(0);
            }
        }

        pid_t pid2 = fork(); // Second child for right command
        if (pid2 == 0)
        {
            // Redirect stdin to pipe's read end
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[1]); // Close unused write end
            close(pipefd[0]);

            // "cd" command: change directory
            if (strcmp(args_right[0], "cd") == 0)
            {
                char path[100];
                if (strlen(args_right[1]) > 0)
                {
                    // If given use the given path
                    strcpy(path, args_right[1]);
                }
                else
                {
                    char *home = getenv("HOME");
                    strcpy(path, home); // If no path given, go to HOME
                }

                int res = chdir(path); // Change directory to path

                // If chdir fails exit.
                if (res == -1)
                {
                    printf("Error at cd %s\n", args_right[1]);
                    exit(-1);
                }

                setenv("PWD", path, 1); // Update PWD environment variable
                exit(0);
            }
            // "pwd" command: print current working directory
            else if (strcmp(args_right[0], "pwd") == 0)
            {
                char pwd2[1024];
                getcwd(pwd2, 1024);
                exit(0);
            }
            // "history" command: show command history
            else if (strcmp(args_right[0], "history") == 0)
            {
                print_history();
                exit(0);
            }
            else
            {
                // System command on right side
                process_system_command(buffer, true);
                exit(0);
            }
        }

        // Parent closes pipe and waits for both children
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
}

// This function is a signal handler for the SIGCHLD signal,
// which is sent to a process when one of its child processes terminates.
// Prevents zombie processes from accumulating when children exit.
void handle_sigchld(int sig) {
    // Reap all dead children
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[])
{
    // Set up signal handler for SIGCHLD to prevent zombie processes.
    // When a child process terminates, this will call handle_sigchld()
    // to reap the child process without blocking the main shell.
    signal(SIGCHLD, handle_sigchld);

    // Start an infinite loop to simulate a shell prompt
    while (1)
    {
        // Print the custom shell prompt
        printf("\nshell>");

        // Read a line of input from the user into 'buffer'
        // fgets reads at most sizeof(buffer) - 1 characters
        if (fgets(buffer, sizeof(buffer), stdin) != NULL)
        {
            // Add the command to shell history (to be used with `history` command)
            add_history(buffer);
        }
        else
        {
            // If input fails show an error message
            printf("Error reading input.\n");
        }

        // trim newline character from the input string,
        // so that it doesn't interfere with parsing and execution.
        trim_newline(buffer);

        // Parse the user input and fill the 'args' and 'args_right' arrays,
        // while also setting flags like has_pipe and has_and if needed
        parse_command(buffer);

        // Execute the parsed command, handling piping or conditional execution
        execute_command(buffer);

        // After executing, reset the args and flags for the next command
        memset(args, 0, sizeof(args)); // Clear left-hand command args
        memset(args_right, 0, sizeof(args_right)); // Clear right-hand command args (for pipe or &&)
        has_pipe = false; // Reset pipe flag
        has_and = false; // Reset '&&' flag
    }
}