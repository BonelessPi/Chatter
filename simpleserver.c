#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define RECV_BUFSIZE 65536
#define BACKLOG 10

struct __attribute__((__packed__)) header {
    int magicNumber;
    int length;
};

struct threadArgs {
    int clientsockfd;
};

void* serviceClient(void* pargs) {
    struct threadArgs* args = (struct threadArgs*)pargs;
    struct header header;
    int clientsockfd = args->clientsockfd;
    free(args);
    recv(clientsockfd, &header, sizeof(struct header), 0);
    header.length = ntohl(header.length);
    header.magicNumber = ntohl(header.magicNumber);
    char* msg = (char*)malloc(header.length);
    recv(clientsockfd, msg, header.length, 0);
    printf("Magic number: %i, Message: %s\n", header.magicNumber, msg);
    free(msg);
    fflush(stdout);
    close(clientsockfd); // Close/clean up socket

    pthread_exit(NULL); // Clean up thread
}

int main(int argc, char** argv) {
    // Step 1: Parse Parameters and initialize variables
    struct addrinfo hints;
    struct addrinfo* info;
    char* port = "60000";

    // Step 2: Find address information of domain and attempt to open socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // OK to use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //Using TCP
    hints.ai_flags = AI_PASSIVE; // Use my IP (extremely important!!)
    
    getaddrinfo(NULL, port, &hints, &info);
    int sockfd = -1;
    struct addrinfo* node = info;
    while (node != NULL && sockfd == -1) {
        printf("Trying socket\n");
        sockfd = socket(node->ai_family, node->ai_socktype, node->ai_protocol); // NOTE: Not bound to port yet
        if (sockfd == -1) {
            fprintf(stderr, "Error on socket...trying another one\n");
            node = node->ai_next;
        }
        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            fprintf(stderr, "Error number %i setting socket options\n", errno);
            exit(errno);
        }
        else if (bind(sockfd, node->ai_addr, node->ai_addrlen) == -1) {
            fprintf(stderr, "Error number %i binding socket\n", errno);
            exit(errno);
        }
    }
    freeaddrinfo(info);

    // Step 3: Service requests (single threaded for now)
    if (sockfd == -1 || node == NULL) {
        printf("ERROR: Error number %i on opening socket\n", errno);
        exit(errno);
    }

    printf("Finished binding. Waiting for connections, sockfd = %i...\n", sockfd);
    if (listen(sockfd, BACKLOG) == -1) {
        fprintf(stderr, "Error number %i listening\n", errno);
        exit(errno);
    }
    
    while (1) {
        struct sockaddr_storage their_addr;
        socklen_t len = sizeof(their_addr);
        struct threadArgs* args = (struct threadArgs*)malloc(sizeof(struct threadArgs));
        args->clientsockfd = accept(sockfd, (struct sockaddr*)&their_addr, &len);
        pthread_t thread;
        printf("Got a connection!  Servicing now...\n");
        fflush(stdout);
        int res = pthread_create(&thread, NULL, serviceClient, (void*)args);
        if (res == -1) {
            fprintf(stderr, "Error %i creating thread", errno);
            exit(errno);
        }
    }



    close(sockfd);
}