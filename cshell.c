
/*
 *  C Shell - A Simple Unix Shell Implementation
 *
 * Overview:
 * This code implements a basic Unix shell, providing functionality to execute commands,
 * manage directories, handle output redirection, and support command pipelines. It 
 * demonstrates essential shell features and is intended for educational purposes to 
 * understand shell behavior and process management in Unix-like operating systems.
 *
 * Features:
 * - **Command Execution**: Executes commands with support for arguments and options.
 * - **Directory Management**: Handles directory changes with the `cd` command and default 
 *   to the user's home directory if no arguments are provided.
 * - **Redirection**: Supports output redirection using the `>` symbol to write command 
 *   output to a specified file.
 * - **Pipes**: Allows piping between commands using the `|` symbol to pass the output of 
 *   one command as input to another.
 * - **Parallel and Sequential Execution**: Supports executing commands in parallel with 
 *   `&&` and sequentially with `##`.
 * - **Signal Handling**: Handles interrupt (`SIGINT`) and stop (`SIGTSTP`) signals to 
 *   ensure proper shell behavior when interrupted or stopped.
 * 
 * Usage:
 * - The shell reads input from the user, parses the command, and executes it according to 
 *   the specified directives.
 * - Special commands:
 *   - `cd <directory>`: Changes the current directory to `<directory>`. Defaults to the 
 *     user's home directory if no argument is provided.
 *   - `exit`: Exits the shell.
 * - Commands can be executed with output redirection, e.g., `command > file.txt`, and 
 *   with pipes, e.g., `cmd1 | cmd2`.
 *
 * Code Structure:
 * - **Includes and Definitions**: Includes necessary headers and defines constants for 
 *   command symbols, maximum values, and error messages.
 * - **Data Structures**: Defines the `_xcmf` structure for holding parsed command details 
 *   and the `execution_directive_t` enum for specifying execution directives.
 * - **Signal Handlers**: Functions to handle `SIGINT` and `SIGTSTP` signals.
 * - **Pipe Handling**: Functions to manage and reset pipe file descriptors.
 * - **Command Parsing**: Function to parse user input into command frames.
 * - **Directory Management**: Function to change the current directory.
 * - **Command Execution**: Functions to execute commands, handle redirection, and 
 *   manage pipelines.
 * - **String Trimming**: Functions to trim leading and trailing spaces from strings.
 * - **Main Function**: The entry point of the shell, which manages user input, command 
 *   execution, and shell lifecycle.
 *
 * Compilation:
 * - Ensure the code is compiled on a Unix-like system with POSIX support.
 *
 * License:
 * - This code is provided for educational purposes and is free to use and modify.
 *
 * Author:
 * - Surya Narayan
 */



#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>

#if !defined(__linux__) && !defined(__gnu_linux__) && !defined(__unix__)
// This line ensures that the code is compiled only on unix based system
#error This code can only be compiled on a Linux system.
#endif

#ifndef _POSIX_VERSION
// The code extensively uses fork(), execvp() and other system calls, hence POSIX is required to run the code
#error To run this code, POSIX is required.
#endif

#define null NULL

typedef char* __string;

#define ARGSMAX             10      
#define OPTSMAX             10      
#define MAX_PATH_LEN        50000   
#define MAX_CMD_LEN         10000   
#define DEFAULT_HOME_DIR    getenv("HOME")  

#define CMD_CD          "cd"      
#define CMD_EXIT        "exit"     
#define _PARALLEL       "&&"       
#define _SEQUENTIAL     "##"       
#define _REDIRECTION    ">"      
#define _PIPELINE       "|" 

#define STD_PERMISSIONS 0644       // File Permissions required while redirecting 
#define __forever__     1

#ifndef ERR_CODE
#define ERR_CODE
    #define ERR_GETCWD      "Error: Current working directory was not found\n"
    #define ERR_CHDIR       "Error: Was not able to change directory\n"
    #define ERR_FORK        "Error: Frok failed.\n"
    #define ERR_EXECVP      "Error: Execution of command failed.\n"
    #define ERR_MALLOC      "Error: Memory allocation failed.\n"
#endif


typedef struct {
    __string _Command;          
    __uint8_t _Cmf_Len;          
    __string* _Args;             
    __uint8_t _Arg_Cnt;          
    __string* _Opts;             
    __uint8_t _Opt_Cnt;         
    __string redirection_file;  
} _xcmf; // extended command frame


typedef enum {
    INIT,           
    PARALLEL,       
    SEQUENTIAL,    
    TERMINATED, 
    PIPELINE,   
    EXCEPTION       
} execution_directive_t;

static int _pipe_arr[2] = {-1, -1}; // used for pipelining 

bool is_piped = false;

// Parse the command frame and return an execution directive.
 
extern execution_directive_t parseInput(__string* _command, _xcmf* cmd) __attribute_pure__ __nonnull((1,2));

// Change the current directory based on the provided command frame.

extern void changeDirectory(_xcmf command_frame);

// Execute the command based on the provided command frame.

extern void executeCommand(_xcmf command_frame);

void startup();
void handle_sigint(int sig) { 
    char cwd[MAX_PATH_LEN];
    getcwd(cwd, sizeof(cwd));
    printf("\n%s$",cwd);
    fflush(stdout);
}
void handle_sigtstp(int sig) {
    char cwd[MAX_PATH_LEN];
    getcwd(cwd, sizeof(cwd));
    printf("\n%s$", cwd);
    fflush(stdout);
}
void pipeReset();

// Parses the command string and and relevent details to command frame.

execution_directive_t parseInput(__string* _command, _xcmf* cmd) {
   
    cmd->_Command = null;
    cmd->_Cmf_Len = 0;
    cmd->_Args = null;
    cmd->_Arg_Cnt = 0;
    cmd->_Opts = null;
    cmd->_Opt_Cnt = 0;
    cmd->redirection_file = null;

    if (*_command == null) {
        return EXCEPTION;
    }


    __string token = strsep(_command, " ");
    if (token != null) {
        cmd->_Command = strdup(token);
        cmd->_Cmf_Len = strlen(token);
    } else {
        return EXCEPTION;
    }

    cmd->_Args = malloc(ARGSMAX * sizeof(__string));
    cmd->_Opts = malloc(OPTSMAX * sizeof(__string));
    if (cmd->_Args == null || cmd->_Opts == null) {
        free(cmd->_Args);
        free(cmd->_Opts);
        return EXCEPTION;
    }


    while ((token = strsep(_command, " ")) != null) {
        
     
        if (strcmp(token, _PARALLEL) == 0) {
            return PARALLEL;
        }

        if (strcmp(token, _SEQUENTIAL) == 0) {
            return SEQUENTIAL;
        }

        if(strcmp(token, _PIPELINE) == 0) {
            return PIPELINE;
        }


        if (strcmp(token, _REDIRECTION)==0) {
            cmd->_Args[cmd->_Arg_Cnt++] = strdup(token);
            if (cmd->redirection_file != null) {
                free(cmd->redirection_file);
            }
            cmd->redirection_file = strsep(_command, " ");
            
        } 
        else if (token[0] == '-') {
          
            cmd->_Opts[cmd->_Opt_Cnt++] = strdup(token);
        } 
        else {
       
            cmd->_Args[cmd->_Arg_Cnt++] = strdup(token);
        }
    }


    for (int i = 0; cmd->_Command[i] != '\0'; ++i) {
        if (cmd->_Command[i] == '>') {
            __string ptr = cmd->_Command;
            cmd->_Command = malloc((i + 1) * sizeof(char));
            strncpy(cmd->_Command, ptr, i);
            cmd->_Command[i] = '\0';

            if (i != strlen(ptr) - 1) {
                ptr = ptr + i + 1;
                cmd->redirection_file = strdup(ptr);
                if (cmd->redirection_file == null) {
                    return EXCEPTION;
                }
            }
            break;
        }
    }

    return TERMINATED;
}

// This function is used to cahnge the directory based on the input command

void changeDirectory(const _xcmf command_frame) {
    __string path;
    char cwd[MAX_PATH_LEN];
    char abs_pth[MAX_PATH_LEN];

    if (command_frame._Arg_Cnt == 0 || command_frame._Args[0] == null) {
        path = DEFAULT_HOME_DIR;
        if (path == null) {
            return;
        }
    } else if (strcmp(command_frame._Args[0], "..") == 0) {

        if (getcwd(cwd, sizeof(cwd)) != null) {
            int ret = snprintf(abs_pth, sizeof(abs_pth), "%s/..", cwd);
            if (ret >= sizeof(abs_pth)) {
                return;
            }
            path = abs_pth;
        } else {
            perror(ERR_GETCWD);
            return;
        }
    } else {

        if (getcwd(cwd, sizeof(cwd)) != null) {
            int ret = snprintf(abs_pth, sizeof(abs_pth), "%s/%s", cwd, command_frame._Args[0]);
            if (ret >= sizeof(abs_pth)) {
                return;
            }
            path = abs_pth;
        } else {
            perror(ERR_GETCWD);
            return;
        }
    }


    if (chdir(path) != 0) {
        perror(ERR_CHDIR);
    }
}


void executeCommand(_xcmf command_frame) {
    if (command_frame._Command == NULL || strlen(command_frame._Command) == 0) {
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror(ERR_FORK);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (command_frame.redirection_file != NULL) {
            int fd = open(command_frame.redirection_file, O_CREAT | O_WRONLY | O_TRUNC, STD_PERMISSIONS);
            if (fd < 0) {
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);
        }

        char *argv[command_frame._Arg_Cnt + command_frame._Opt_Cnt + 2];
        argv[0] = command_frame._Command;

        int arg_idx = 1;
        for (int i = 0; i < command_frame._Opt_Cnt; ++i) {
            argv[arg_idx++] = command_frame._Opts[i];
        }
        for (int i = 0; i < command_frame._Arg_Cnt; i++) {
            if (strcmp(command_frame._Args[i], _REDIRECTION) == 0) {
                break;
            }
            argv[arg_idx++] = command_frame._Args[i];
        }
        argv[arg_idx] = NULL;

        if (is_piped) {
            close(_pipe_arr[1]);
            dup2(_pipe_arr[0], STDIN_FILENO);
            close(_pipe_arr[0]);
        }

        if (execvp(command_frame._Command, argv) == -1) {
            printf("Shell: Incorrect command\n");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }

    pipeReset();
}

// This function executes the command in pipeline 

void executeCommandPipe(_xcmf command_frame) {
    if (pipe(_pipe_arr) == -1) {
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror(ERR_FORK);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char *argv[command_frame._Arg_Cnt + command_frame._Opt_Cnt + 2];
        argv[0] = command_frame._Command;

        int arg_idx = 1;
        for (int i = 0; i < command_frame._Opt_Cnt; ++i) {
            argv[arg_idx++] = command_frame._Opts[i];
        }
        for (int i = 0; i < command_frame._Arg_Cnt; i++) {
            argv[arg_idx++] = command_frame._Args[i];
        }
        argv[arg_idx] = NULL;

        dup2(_pipe_arr[1], STDOUT_FILENO);
        close(_pipe_arr[1]);

        if (execvp(command_frame._Command, argv) == -1) {
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else {
        close(_pipe_arr[1]); 

        int status;
        waitpid(pid, &status, 0);

        is_piped = true;
    }
}




void leftTrim(__string str) {
    const int len = strlen(str);
    int i = 0;
    while (str[i] == ' ' && i < len) {
        i++;
    }
    if (i > 0) {
        memmove(str, str + i, len - i + 1);
    }
}


void rightTrim(__string str) {
    const int len = strlen(str);
    int last_idx = len - 1;

    while (last_idx >= 0 && str[last_idx] == ' ') {
        str[last_idx] = '\0';
        last_idx--;
    }
}


// Main function to run the shell.

int main() {
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    startup();
    pipeReset();


    char *line = NULL;
    size_t len = 0;
    size_t read;

    while (__forever__) {
        char cwd[MAX_PATH_LEN];
        getcwd(cwd, sizeof(cwd));
        printf("%s$", cwd);

        if ((read = getline(&line, &len, stdin)) == -1) {
            perror("getline");
            free(line);
            exit(EXIT_FAILURE);
        }

        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        
        leftTrim(line);
        rightTrim(line);

        execution_directive_t directive;
        do {
            _xcmf cmd;
            directive = parseInput(&line, &cmd);

            if (strcmp(cmd._Command, CMD_CD) == 0) {
                changeDirectory(cmd);
            } 
            else if (strcmp(cmd._Command, CMD_EXIT) == 0) {
                printf("Exiting shell...\n");
                free(cmd._Command);
                for (int i = 0; i < cmd._Arg_Cnt; i++) {
                    free(cmd._Args[i]);
                }
                free(cmd._Args);
                for (int i = 0; i < cmd._Opt_Cnt; i++) {
                    free(cmd._Opts[i]);
                }
                free(cmd._Opts);
                free(line);
                return 0;
            } 
            else {
                if (directive == PARALLEL) {
                    const pid_t pid = fork();
                    if (pid == 0) {
                        executeCommand(cmd);
                        free(cmd._Command);
                        for (int i = 0; i < cmd._Arg_Cnt; i++) {
                            free(cmd._Args[i]);
                        }
                        free(cmd._Args);
                        for (int i = 0; i < cmd._Opt_Cnt; i++) {
                            free(cmd._Opts[i]);
                        }
                        free(cmd._Opts);
                        exit(EXIT_SUCCESS);
                    }
                    if (pid > 0) {
                        continue;
                    }
                    perror(ERR_FORK);
                    break;
                }
                else if(directive == PIPELINE) {
                    executeCommandPipe(cmd);
                }
                else {
                    executeCommand(cmd);
                }
                
                
            }

            free(cmd._Command);
            for (int i = 0; i < cmd._Arg_Cnt; i++) {
                free(cmd._Args[i]);
            }
            free(cmd._Args);
            for (int i = 0; i < cmd._Opt_Cnt; i++) {
                free(cmd._Opts[i]);
            }
            free(cmd._Opts);

        } while (directive != TERMINATED && directive != EXCEPTION);

        if (directive == PARALLEL) {
            int status;
            while (wait(&status) > 0);
        }
    }

    return 0;
}

void pipeReset() {
    is_piped = false;
    if (_pipe_arr[0] != -1) {
        close(_pipe_arr[0]);
        _pipe_arr[0] = -1;
    }
    if (_pipe_arr[1] != -1) {
        close(_pipe_arr[1]);
        _pipe_arr[1] = -1;
    }
}

void startup() {
    pipeReset();
    printf("\n");
    printf("  \\\\         ____      ____ _           _ _\n");
    printf("   \\\\       / ___|    / ___| |___  ___ | | |\n");
    printf("    \\\\     | |        \\___ |  _  |/ _ \\| | |\n");
    printf("  ((//     | |___      ___)| | | |  __/| | |\n");
    printf("   //       \\____)    |____|_| |_|\\___/|_|_|   ___________\n");
    printf("  // =========================================================\n\n\n");
}