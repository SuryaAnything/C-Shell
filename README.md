# C Shell - A Simple Unix Shell Implementation

## Overview
This project implements a basic Unix shell with essential functionalities such as command execution, directory management, output redirection, and command pipelines. It is designed for educational purposes, allowing users to understand the behavior of shells and process management in Unix-like operating systems.

## Features
- **Command Execution**: Run commands with support for arguments and options.
- **Directory Management**: Change directories using the `cd` command, with support for the default user's home directory.
- **Redirection**: Redirect output to files using the `>` symbol.
- **Pipes**: Pipe output from one command to another using the `|` symbol.
- **Parallel and Sequential Execution**: Run commands in parallel using `&&` and sequentially using `##`.
- **Signal Handling**: Handles `SIGINT` (interrupt) and `SIGTSTP` (stop) signals for proper shell behavior.

## Usage
- The shell reads input from the user, parses the command, and executes it according to specified directives.
- Special commands:
  - `cd <directory>`: Changes the current directory to `<directory>`. Defaults to the user's home directory if no argument is provided.
  - `exit`: Exits the shell.
- Commands can be executed with output redirection, e.g., `command > file.txt`, and with pipes, e.g., `cmd1 | cmd2`.

## Code Structure
- **Includes and Definitions**: Necessary headers and constants for command symbols, maximum values, and error messages.
- **Data Structures**: `_xcmf` structure for holding parsed command details and `execution_directive_t` enum for specifying execution directives.
- **Signal Handlers**: Functions to handle `SIGINT` and `SIGTSTP` signals.
- **Pipe Handling**: Functions to manage and reset pipe file descriptors.
- **Command Parsing**: Function to parse user input into command frames.
- **Directory Management**: Function to change the current directory.
- **Command Execution**: Functions to execute commands, handle redirection, and manage pipelines.
- **String Trimming**: Functions to trim leading and trailing spaces from strings.
- **Main Function**: The entry point of the shell, which manages user input, command execution, and shell lifecycle.

## Compilation
Ensure the code is compiled on a Unix-like system with POSIX support.

## License
This code is provided for educational purposes and is free to use and modify.

## Author
- Surya Narayan
