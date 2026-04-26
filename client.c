#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#define MAX 512

int main(int argc, char *argv[]) {
    int sock, port;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char buffer[MAX];

    if (argc < 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[2]);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname(argv[1]);

    if (server == NULL) {
        fprintf(stderr, "Error: No such host\n");
        exit(0);
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy(server->h_addr, &server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        exit(1);
    }

    while (1) {
        bzero(buffer, MAX);
        int n = recv(sock, buffer, MAX - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        printf("%s", buffer);

        // ✅ FIXED INPUT TRIGGER
        if (strstr(buffer, "IGN") || strstr(buffer, "Choose action")) {
            printf("> ");

            bzero(buffer, MAX);
            fgets(buffer, MAX, stdin);

            buffer[strcspn(buffer, "\n")] = 0;

            if (strlen(buffer) == 0) strcpy(buffer, " ");

            send(sock, buffer, strlen(buffer), 0);
        }
    }

    printf("\nDisconnected from server.\n");
    close(sock);
    return 0;
}
