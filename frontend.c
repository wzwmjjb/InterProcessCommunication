#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX 128
#define PIPE_PATH "/tmp/myfifo"

// Message queue structure
struct mesg_buffer {
    long mesg_type;
    char mesg_text[100];
} message;

// Function to translate Windows commands to Linux commands
const char *translate_command(const char *command) {
    char *translatedCommand = malloc(strlen(command) + 1); // Allocate memory for the translated command
    if (!translatedCommand) {
        return NULL; // Return NULL if memory allocation fails
    }

    if (strncmp(command, "dir", 3) == 0) {
        // Replace 'dir' with 'ls'
        strcpy(translatedCommand, "ls");
        strcat(translatedCommand, command + 3); // Append the rest of the command
    } else if (strncmp(command, "rename", 6) == 0) {
        // Replace 'rename' with 'mv'
        strcpy(translatedCommand, "mv");
        strcat(translatedCommand, command + 6); // Append the rest of the command
    } else if (strncmp(command, "move", 4) == 0) {
        // 'move' command is the same as 'mv' in Linux
        strcpy(translatedCommand, "mv");
        strcat(translatedCommand, command + 4); // Append the rest of the command
    } else if (strncmp(command, "del", 3) == 0) {
        // Replace 'del' with 'rm'
        strcpy(translatedCommand, "rm");
        strcat(translatedCommand, command + 3); // Append the rest of the command
    } else if (strcmp(command, "exit") == 0) {
        // 'exit' command is the same in both Windows and Linux
        strcpy(translatedCommand, command);
    } else {
        // For commands like 'cd', which are the same in both Windows and Linux
        printf("The command line is not currently supported :(\n");
        return NULL;
    }

    return translatedCommand;
}

int main() {
    int msgid;
    pid_t pid;

    // Create message queue
    msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    // Convert msgid to string
    char msgid_string[10];
    sprintf(msgid_string, "%d", msgid);
    message.mesg_type = 1;

    // Create the named pipe
    mkfifo(PIPE_PATH, 0666);

    printf("MicroSoft Windows\n\n");


    // fork
    pid = fork();

    if (pid < 0) {
        printf("Failed to fork\n");
        exit(1);
    }
    if (pid == 0) {
        // Child process
        execl("./backend", "backend", msgid_string, NULL);
        exit(0);
    } else {
        // Parent process
        char input[MAX];
        while (1) {
            printf("[IN] : ");
            fgets(input, MAX, stdin);
            input[strcspn(input, "\n")] = 0; // Remove newline

            // Translate command
            const char *command = translate_command(input);
            if (command == NULL) continue;

            // Send command to backend
            strcpy(message.mesg_text, command);
            msgsnd(msgid, &message, sizeof(message), 0);

            // Exit if command is 'exit'
            if (strcmp(command, "exit") == 0) {
                break;
            }

            // Read response from backend
            int fd = open(PIPE_PATH, O_RDONLY);
            char output[1000];
            read(fd, output, 1000);
            printf("[OUT] :\n%s\n", output);
            close(fd);
        }

        // Wait for child process to finish
        wait(NULL);

        // Destroy message queue
        msgctl(msgid, IPC_RMID, NULL);

        // Remove the named pipe
        unlink(PIPE_PATH);
    }
    return 0;
}

