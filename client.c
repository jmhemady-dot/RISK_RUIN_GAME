#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>

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

    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);          // listen to server
        FD_SET(STDIN_FILENO, &readfds);  // listen to keyboard

        int maxfd = (sock > STDIN_FILENO ? sock : STDIN_FILENO) + 1;

        if (select(maxfd, &readfds, NULL, NULL, NULL) < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(sock, &readfds)) {
            int n = recv(sock, buffer, MAX - 1, 0);
            if (n <= 0) break;

            buffer[n] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
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
