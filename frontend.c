#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX 256
#define PIPE_PATH "/tmp/myfifo"

// Message queue structure
struct mesg_buffer {
    long mesg_type;
    char mesg_text[100];
} message;

// Function to translate Windows commands to Linux commands
const char *translate_command(const char *command) {
    if (strcmp(command, "dir") == 0) return "ls";
    if (strcmp(command, "rename") == 0 || strcmp(command, "move") == 0) return "mv";
    if (strcmp(command, "del") == 0) return "rm";
    if (strcmp(command, "cd") == 0)
        return "pwd";  // Note: 'cd' command will not change the directory of the backend process
    if (strcmp(command, "exit") == 0) return "exit";
    return command;
}

int main() {
    int msgid;
    pid_t pid;

    // Create message queue
    msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    printf("Message queue ID: %d\n", msgid);
    // Convert msgid to string
    char msgid_string[10];
    sprintf(msgid_string, "%d", msgid);
    message.mesg_type = 1;

    // Create the named pipe
    mkfifo(PIPE_PATH, 0666);

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
            printf("Enter command: ");
            fgets(input, MAX, stdin);
            input[strcspn(input, "\n")] = 0; // Remove newline

            // Translate command
            const char *command = translate_command(input);

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
            printf("Output:\n%s\n", output);
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

