#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define PORT "3000"
#define BUFFER_SIZE 1024

int main(void) {

    int rc, sock_fd;
    struct addrinfo hints, *server_info, *i;

    struct sockaddr client_addr;
    socklen_t client_addrlen;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rc = getaddrinfo(NULL, PORT, &hints, &server_info)) != 0) {
        fprintf(stderr, "server: %s\n", gai_strerror(rc));
        exit(1);
    }

    // loop through all candidate addresses and bind to the first we can
    for (i = server_info; i != NULL; i = i->ai_next) {
        sock_fd = socket(server_info->ai_family, server_info->ai_socktype, 0);

        if (sock_fd == -1) {
            continue;
        }

        // Set SO_REUSEADDR option
        int yes = 1;
        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        rc = bind(sock_fd, server_info->ai_addr, server_info->ai_addrlen);
        if (rc == -1) {
            close(sock_fd);
            continue;
        }

        break;
    }

    freeaddrinfo(server_info);  // done using this struct

    if (i == NULL) {
        fprintf(stderr, "server: %s\n", strerror(errno));
        exit(1);
    }


    if (listen(sock_fd, SOMAXCONN) == -1) {
        fprintf(stderr, "server: %s\n", strerror(errno));
        exit(1);
    }

    printf("server: listening to connections on port %s\n", PORT);

    int next_client = 1;

    // the main loop of the web server
    while (1) {
        int client_fd = accept(sock_fd, &client_addr, &client_addrlen);
        if (client_fd == -1) {
            fprintf(stderr, "server: %s\n", strerror(errno));
            continue;
        }

        int client_id = next_client++;

        printf("server: client %d connected\n", client_id);

        while (1) {
            char buff[BUFFER_SIZE];
            ssize_t ret = recv(client_fd, &buff, BUFFER_SIZE, 0);
            if (ret == -1) {
                fprintf(stderr, "server: failed to receive buffer from client %d\n", client_id);
                close(client_fd);
                break;
            }

            if (ret == 0) {
                close(client_fd);
                printf("server: client %d disconnected\n", client_id);
                break;
            }

            send(client_fd, buff, strlen(buff), 0);
            memset(buff, 0, BUFFER_SIZE);
        }

    }

    return 0;
}
