#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define COMMAND_MAX 128
#define OUTPUT_MAX 1024
#define PIPE_PATH "/tmp/my_fifo"

// Message queue structure
struct msg_buffer {
    long msg_type;
    char msg_text[COMMAND_MAX];
} message;

int main(int argc, char *argv[]) {
    int msg_id;
    FILE *fp;
    char path[OUTPUT_MAX];
    int fd;

    // Access the message queue
    if (argc < 2) {
        return 1;
    }

    // Convert string to msg_id
    msg_id = atoi(argv[1]);

    while (1) {
        // Receive command from frontend
        int msg_rcv_status = msgrcv(msg_id, &message, sizeof(message), message.msg_type, 0);
        if (msg_rcv_status < 0) {
            perror("Failed to receive message\n");
            exit(1);
        }

        // Exit backend if command is 'exit'
        if (strcmp(message.msg_text, "exit") == 0) {
            break;
        }

        // Execute the command
        if (strcmp(message.msg_text, "cd") == 0) {
            chdir(getenv("HOME"));
            fp = popen("pwd", "r");
        } else if (strncmp(message.msg_text, "cd", 2) == 0) {
            chdir(message.msg_text + 3);
            fp = popen("pwd", "r");
        } else {
            fp = popen(message.msg_text, "r");
        }

        if (fp == NULL) {
            perror("Failed to run command\n");
            exit(1);
        }

        // Send output back to frontend
        fd = open(PIPE_PATH, O_WRONLY);
        if (fd < 0) {
            perror("Failed to open pipe\n");
            exit(1);
        }

        while (fgets(path, sizeof(path), fp) != NULL) {
            ssize_t bytes_written = write(fd, path, strlen(path));
            if (bytes_written < 0) {
                perror("Failed to write to pipe\n");
                exit(1);
            }
        }
        close(fd);
        pclose(fp);
    }

    return 0;
}

