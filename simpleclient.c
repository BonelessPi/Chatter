#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

struct __attribute__((__packed__)) header {
    int magicNumber;
    int length;
};

int main(int argc, char** argv) {
    // Step 1: Get address information for a host
    struct addrinfo hints;
    struct addrinfo* node;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // Use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP
    int ret = getaddrinfo("192.168.50.59", "60000", &hints, &node);
    if (ret != 0) {
        fprintf(stderr, "Error number %i getting address info\n", ret);
        freeaddrinfo(node);
        exit(ret);
    }
    int sockfd = -1;
    // Step 1b: Try all possible connection types in the link list
    // it gives me until I find one that works
    while (node != NULL) {
        sockfd = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
        if (sockfd != -1) {
            break;
        }
        else {
            node = node->ai_next;
        }
    }
    // Step 1c: Make sure we got a valid socket file descriptor
    // after going through all of the options
    if (sockfd == -1) {
        fprintf(stderr, "Error number %i opening socket\n", errno);
        freeaddrinfo(node);
        exit(errno);
    }
    // Step 2: Setup stream on socket and connect
    ret = connect(sockfd, node->ai_addr, node->ai_addrlen);
    freeaddrinfo(node);
    if (ret == -1) {
        fprintf(stderr, "Error number %i connecting to socket\n", errno);
        exit(errno);
    }

    // Step 3: Send message
    char* payload = "Hello!  This is my greeting from the client to the server";
    // Sent header
    struct header header;
    header.magicNumber = htonl(475);
    int len = strlen(payload)+1;
    header.length = htonl(len);
    int bytesSent = send(sockfd, &header, sizeof(struct header), 0);
    printf("%i bytes sent for header (tried to send %ld)\n", bytesSent, sizeof(struct header));
    bytesSent = send(sockfd, payload, len, 0);
    printf("%i bytes sent for payload (tried to send %i)\n", bytesSent, len);

    close(sockfd);
}