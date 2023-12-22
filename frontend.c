#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define COMMAND_MAX 128
#define OUTPUT_MAX 1024
#define PIPE_PATH "/tmp/my_fifo"

// Message queue structure
struct msg_buffer {
    long msg_type;
    char msg_text[COMMAND_MAX];
} message;

// Translate Windows commands to Linux commands
const char *translate_command(const char *command) {
    char *translatedCommand = malloc(strlen(command) + 1); // Allocate memory for the translated command
    if (!translatedCommand) {
        return NULL;
    }

    if (strncmp(command, "dir", 3) == 0) {
        // dir -> ls
        strcpy(translatedCommand, "ls");
        strcat(translatedCommand, command + 3); // Append the rest of the command
    } else if (strncmp(command, "rename", 6) == 0) {
        // rename -> mv
        strcpy(translatedCommand, "mv");
        strcat(translatedCommand, command + 6);
    } else if (strncmp(command, "cd", 2) == 0) {
        // cd -> cd(pwd)
        strcpy(translatedCommand, "pwd");
        strcat(translatedCommand, command + 2);
    } else if (strncmp(command, "move", 4) == 0) {
        // move -> mv
        strcpy(translatedCommand, "mv");
        strcat(translatedCommand, command + 4);
    } else if (strncmp(command, "del", 3) == 0) {
        // del -> rm
        strcpy(translatedCommand, "rm");
        strcat(translatedCommand, command + 3);
    } else if (strcmp(command, "exit") == 0) {
        // exit -> exit
        strcpy(translatedCommand, command);
    } else {
        // Command not supported
        printf("\033[31mThe command line is not currently supported :(\033[0m\n");
        return NULL;
    }
    printf("\033[32m[IN_LINUX]\033[0m: %s\n", translatedCommand);
    return translatedCommand;
}

// Remove pipe if it exists
void remove_pipe() {
    if (access(PIPE_PATH, F_OK) != -1) {
        unlink(PIPE_PATH);
        printf("Successfully removed pipe\n");
    }
}

int main() {
    int msg_id;
    pid_t pid;

    printf("\033[32mMicroSoft Windows\033[0m\n\n");

    // Create message queue
    msg_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msg_id < 0) {
        perror("Failed to create message queue\n");
        exit(1);
    }
    // Convert msg_id to string
    char msg_id_string[10];
    sprintf(msg_id_string, "%d", msg_id);
    message.msg_type = 1;

    // Remove pipe if it exists
    remove_pipe();

    // Create the named pipe
    int status = mkfifo(PIPE_PATH, 0666);
    if (status < 0) {
        perror("Failed to create named pipe\n");
        exit(1);
    }

    // fork
    pid = fork();

    if (pid < 0) {
        printf("Failed to fork\n");
        exit(1);
    }
    if (pid == 0) {
        // Child process
        execl("./backend", "backend", msg_id_string, NULL);
        exit(0);
    } else {
        // Parent process
        char input[COMMAND_MAX];
        while (1) {
            printf("\033[32m[IN]\033[0m: ");
            fgets(input, COMMAND_MAX, stdin);
            input[strcspn(input, "\n")] = 0; // Remove newline

            // Translate command
            const char *command = translate_command(input);
            if (command == NULL) {
                continue;
            }
            // Send command to backend
            strcpy(message.msg_text, command);
            msgsnd(msg_id, &message, sizeof(message), 0);

            // Exit frontend if command is 'exit'
            if (strcmp(command, "exit") == 0) {
                break;
            }

            // Read response from backend
            int fd = open(PIPE_PATH, O_RDONLY);
            if (fd < 0) {
                perror("Failed to open pipe\n");
                exit(1);
            }
            char output[OUTPUT_MAX];
            printf("\033[34m[OUT]\033[0m:\n");
            ssize_t read_bytes;
            while ((read_bytes = read(fd, output, sizeof(output) - 1)) > 0) {
                output[read_bytes] = '\0';
                printf("%s", output);
            }
            close(fd);
        }

        // Wait for child process to finish
        wait(NULL);

        // Destroy message queue
        msgctl(msg_id, IPC_RMID, NULL);

        // Remove the named pipe
        unlink(PIPE_PATH);
    }
    return 0;
}

