#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX 80
#define PIPE_PATH "/tmp/myfifo"
#define MSG_QUEUE_KEY 1234

// Message queue structure
struct mesg_buffer {
    long mesg_type;
    char mesg_text[100];
} message;

int main(int argc, char *argv[]) {
    int msgid;
    FILE *fp;
    char path[1000];

    // Access the message queue
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <msgid>\n", argv[0]);
        return 1;
    }

    // Convert string to msgid
    msgid = atoi(argv[1]);

    while (1) {
        // Receive command from frontend
        msgrcv(msgid, &message, sizeof(message), 1, 0);

        // Exit if command is 'exit'
        if (strcmp(message.mesg_text, "exit") == 0) {
            break;
        }

        // Execute the command
        fp = popen(message.mesg_text, "r");
        if (fp == NULL) {
            printf("Failed to run command\n" );
            exit(1);
        }

        // Send output back to frontend
        int fd = open(PIPE_PATH, O_WRONLY);
        while (fgets(path, sizeof(path), fp) != NULL) {
            write(fd, path, strlen(path));
        }
        close(fd);
        pclose(fp);
    }

    return 0;
}

