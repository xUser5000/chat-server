#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#define MSG_SIZE 1024
#define MAX_CLIENTS 512

struct client_t {
    int socket_fd;
    int id;
};

struct client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_guard;

int create_server(char *port);

void *handle_client(void *args);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: server <port>\n");
        exit(1);
    }

    char *port = argv[1];

    int server_sock_fd = create_server(port);
    printf("server: listening to connections on port %s\n", port);

    struct sockaddr client_addr;
    socklen_t client_addrlen;
    int next_client_id = 1;
    while (1) {
        int client_fd = accept(server_sock_fd, &client_addr, &client_addrlen);
        if (client_fd == -1) {
            fprintf(stderr, "server: %s\n", strerror(errno));
            continue;
        }

        pthread_mutex_lock(&clients_guard);
        int idx = -1;
        for (int c = 0; c < MAX_CLIENTS; c++) {
            if (clients[c] == NULL) {
                idx = c;
            }
        }

        if (idx == -1) {
            pthread_mutex_unlock(&clients_guard);
            close(client_fd);
            fprintf(stderr, "server: max no. clients reached\n");
            continue;
        }

        struct client_t *curr_client = (struct client_t *) malloc(sizeof(struct client_t));
        curr_client->id = next_client_id++;
        curr_client->socket_fd = client_fd;
        clients[idx] = curr_client;
        pthread_mutex_unlock(&clients_guard);

        printf("server: client %d connected\n", curr_client->id);

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        pthread_t handler;
        pthread_create(&handler, &attr, handle_client, curr_client);
    }

}

int create_server(char *port) {
    int rc, sock_fd;
    struct addrinfo hints, *server_info, *i;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rc = getaddrinfo(NULL, port, &hints, &server_info)) != 0) {
        fprintf(stderr, "server: %s\n", gai_strerror(rc));
        exit(1);
    }

    /* loop through all candidate addresses and bind to the first available one */
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

    return sock_fd;
}

void *handle_client(void *args) {
    struct client_t *client = (struct client_t *) args;

    char prompt[20];
    char msg[MSG_SIZE];
    while (1) {

        ssize_t ret = recv(client->socket_fd, &msg, MSG_SIZE, 0);
        if (ret == -1) {
            fprintf(stderr, "server: failed to receive buffer from client %d\n", client->id);
            close(client->socket_fd);
            break;
        }

        if (ret == 0) {
            printf("server: client %d disconnected\n", client->id);
            close(client->socket_fd);
            break;
        }

        /* send to all connected clients except the original sender */
        pthread_mutex_lock(&clients_guard);
        for(struct client_t **ptr = clients; ptr < clients + MAX_CLIENTS; ptr++)
        {
            if (*ptr != NULL && (*ptr)->id != client->id) {
                sprintf(prompt, "client %d> ", client->id);
                send((*ptr)->socket_fd, prompt, strlen(prompt), 0);
                send((*ptr)->socket_fd, msg, strlen(msg), 0);
            }
        }
        pthread_mutex_unlock(&clients_guard);

        memset(msg, 0, MSG_SIZE);
    }

    /* free the client's resources */
    pthread_mutex_lock(&clients_guard);
    for(struct client_t **ptr = clients; ptr < clients + MAX_CLIENTS; ptr++)
    {
        if (*ptr != NULL && (*ptr)->id == client->id) {
            *ptr = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_guard);
    free(client);
    return NULL;
}
