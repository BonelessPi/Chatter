#include <ncurses.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "linkedlist.h"
#include "hashmap.h"
#include "arraylist.h"
#include "chatter.h"

#define BACKLOG 20

///////////////////////////////////////////////////////////
//       Data Structure Memory Management
///////////////////////////////////////////////////////////

struct ReceiveData {
    struct Chatter* chatter;
    struct Chat* chat;
};

struct Chat* initChat(int sockfd) {
    struct Chat* chat = (struct Chat*)malloc(sizeof(struct Chat));
    chat->messagesIn = LinkedList_init();
    chat->messagesOut = LinkedList_init();
    chat->outCounter = 0;
    chat->sockfd = sockfd;
    return chat;
}

void destroyChat(struct Chat* chat) {
    freeMessages(chat->messagesIn);
    LinkedList_free(chat->messagesIn);
    freeMessages(chat->messagesOut);
    LinkedList_free(chat->messagesOut);
    free(chat);
}

void freeMessages(struct LinkedList* messages) {
    struct LinkedNode* node = messages->head;
    while (node != NULL) {
        struct Message* message = (struct Message*)node->data;
        free(message->text);
        free(message);
        node = node->next;
    }
}

struct Chatter* initChatter() {
    // Dynamically allocate all objects that need allocating
    struct Chatter* chatter = (struct Chatter*)malloc(sizeof(struct Chatter));
    chatter->gui = initGUI();
    strcpy(chatter->myname, "Anonymous");
    chatter->chats = LinkedList_init();
    chatter->activeChat = NULL;
    pthread_mutex_init(&chatter->lock, NULL);
    return chatter;
}

void destroyChatter(struct Chatter* chatter) {
    destroyGUI(chatter->gui);
    struct LinkedNode* chatNode = chatter->chats->head;
    while (chatNode != NULL) {
        struct Chat* chat = (struct Chat*)chatNode->data;
        destroyChat(chat);
        chatNode = chatNode->next;
    }
    LinkedList_free(chatter->chats);
    pthread_mutex_destroy(&chatter->lock);
    free(chatter);
}

/**
 * @brief Get the first chat object in a list that corresponds
 * to a name
 * (NOTE: This could be done more efficiently with a hash table,
 * but we'll used linked lists for now)
 * 
 * @param name String name
 * @return struct Chat* 
 */
struct Chat* getChatFromName(struct Chatter* chatter, char* name) {
    struct Chat* chat = NULL;
    pthread_mutex_lock(&chatter->lock);
    struct LinkedNode* node = chatter->chats->head;
    int len = strlen(name);
    while (node != NULL) {
        struct Chat* thisChat = (struct Chat*)node->data;
        if (strncmp(name, chat->name, len) == 0) {
            chat = thisChat;
            break;
        }
        node = node->next;
    }
    pthread_mutex_unlock(&chatter->lock);
    return chat;
}


///////////////////////////////////////////////////////////
//             Chat Session Messages In
///////////////////////////////////////////////////////////

/**
 * @brief Continually loop and receive info
 * 
 * @param pargs 
 * @return void* 
 */
void* receiveLoop(void* pargs) {
    struct ReceiveData* param = (struct ReceiveData*)pargs;
    struct Chat* chat = param->chat;
    struct Chatter* chatter = param->chatter;
    free(param);
    while (1) {
        struct header_generic header;
        if (recv(chat->sockfd, &header, sizeof(struct header_generic), 0) == 0) {
            break;
        }
        // TODO: Fill this in.  Cast the header as appropriate, and receive
        // the rest of the data.  
        // Be sure to lock variables as appropriate for thread safety

        reprintUsernameWindow(chatter);
        reprintChatWindow(chatter);
    }
    return NULL;
}



///////////////////////////////////////////////////////////
//             Chat Session Messages Out
///////////////////////////////////////////////////////////


/**
 * @brief Send a message in the active chat
 * 
 * @param chatter Data about the current chat session
 * @param message 
 */
void sendMessage(struct Chatter* chatter, char* message) {
    // TODO: Fill this in
}

/**
 * @brief Delete message in the active chat
 * 
 * @param chatter Data about the current chat session
 * @param id ID of message to delete
 */
void deleteMessage(struct Chatter* chatter, uint16_t id) {
    // TODO: Fill this in
}

/**
 * @brief Send a file in the active chat
 * 
 * @param chatter Data about the current chat session
 * @param filename Path to file
 */
void sendFile(struct Chatter* chatter, char* filename) {
    // TODO: Fill this in
}

/**
 * @brief Broadcast my name to all active connections
 * NOTE: Name is held in chatter->myname
 * 
 * @param chatter Data about the current chat session
 */
void broadcastMyName(struct Chatter* chatter) {
    // TODO: Fill this in
}

/**
 * @brief Close chat with someone
 * 
 * @param chatter Data about the current chat session
 * @param name Close connection with this person
 */
void closeChat(struct Chatter* chatter, char* name) {
    // TODO: Fill this in
}


/**
 * @brief Switch the active chat
 * 
 * @param chatter Data about the current chat session
 * @param name Switch chat to be with this person
 */
void switchTo(struct Chatter* chatter, char* name) {
    pthread_mutex_lock(&chatter->lock);
    struct Chat* chat = getChatFromName(chatter, name);
    if (chat != NULL) {
        chatter->activeChat = chat;
    }
    pthread_mutex_unlock(&chatter->lock);
}




///////////////////////////////////////////////////////////
//               Connection Management
///////////////////////////////////////////////////////////

/**
 * @brief Print out a socket error, pause, and then exit
 * 
 * @param chatter 
 * @param fmt 
 */
void socketErrorAndExit(struct Chatter* chatter, char* fmt) {
    char* error = (char*)malloc(strlen(fmt) + 100);
    sprintf(error, fmt, errno);
    printErrorGUI(chatter->gui, error);
    free(error);
    sleep(5);
    destroyChatter(chatter);
    exit(errno);
}


/**
 * @brief Setup a chat on a socket, regardless of whether it came from
 * a client or server
 * 
 * @param chatter Chatter object
 * @param sockfd A socket that's already been connected to a stream
 */
void setupNewChat(struct Chatter* chatter, int sockfd) {
    // Step 1: Setup a new chat object and add to the list
    struct Chat* chat = initChat(sockfd);
    strcpy(chat->name, "Anonymous");
    pthread_mutex_lock(&chatter->lock);
    LinkedList_addFirst(chatter->chats, (void*)chat);
    // Step 2: Start a thread for a loop that receives data
    struct ReceiveData* param = (struct ReceiveData*)malloc(sizeof(struct ReceiveData));
    param->chat = chat;
    param->chatter = chatter;
    pthread_t receiveThread;
    int res = pthread_create(&receiveThread, NULL, receiveLoop, (void*)param);
    if (res != 0) {
        // Print out error information
        char* fmt = "Error %i opening new connection";
        char* error = (char*)malloc(strlen(fmt) + 100);
        sprintf(error, fmt, errno);
        printErrorGUI(chatter->gui, error);
        free(error);
        // Remove dynamically allocated stuff
        LinkedList_removeFirst(chatter->chats);
        destroyChat(chat);
    }
    else if (chatter->chats->head->next == NULL) {
        // This is the first chat; make it active
        chatter->activeChat = chat;
    }
    pthread_mutex_unlock(&chatter->lock);
    reprintUsernameWindow(chatter);
    reprintChatWindow(chatter);
}



/**
 * @brief Establish a chat as a client connecting to an IP/port
 * 
 * @param chatter Data about the current chat session
 * @param IP IP address in human readable form
 * @param port Port on which to establish connection
 */
void connectChat(struct Chatter* chatter, char* IP, char* port) {
    // Step 1: Get address information for a host
    struct addrinfo hints;
    struct addrinfo* node;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // Use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP
    int ret = getaddrinfo(IP, port, &hints, &node);
    if (ret != 0) {
        printErrorGUI(chatter->gui, "Error number getting address info");
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
        printErrorGUI(chatter->gui, "Error opening socket");
        freeaddrinfo(node);
        exit(errno);
    }
    // Step 2: Setup stream on socket and connect
    ret = connect(sockfd, node->ai_addr, node->ai_addrlen);
    freeaddrinfo(node);
    if (ret == -1) {
        printErrorGUI(chatter->gui, "Error opening socket");
        exit(errno);
    }
    setupNewChat(chatter, sockfd);
}

/**
 * @brief Continually loop through and accept new connections
 * 
 * @param pargs Pointer to the chatter data
 */
void* serverLoop(void* pargs) {
    struct Chatter* chatter = (struct Chatter*)pargs;
    while (1) {
        struct sockaddr_storage their_addr;
        socklen_t len = sizeof(their_addr);
        int sockfd = accept(chatter->serversock, (struct sockaddr*)&their_addr, &len);
        if (sockfd != -1) {
            setupNewChat(chatter, sockfd);
        }
    }
}


int main(int argc, char *argv[]) {
    char* port = "60000";
    if (argc > 1) {
        port = argv[1];
    }
    // Step 1: Initialize chatter object and setup server to listen for incoming connections
    struct Chatter* chatter = initChatter();
    struct GUI* gui = chatter->gui;
    // Step 1a: Parse Parameters and initialize variables
    struct addrinfo hints;
    struct addrinfo* info;
    // Step 1b: Find address information of domain and attempt to open socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // OK to use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //Using TCP
    hints.ai_flags = AI_PASSIVE; // Use my IP (extremely important!!)
    getaddrinfo(NULL, port, &hints, &info);
    chatter->serversock = -1;
    struct addrinfo* node = info;
    while (node != NULL && chatter->serversock == -1) {
        chatter->serversock = socket(node->ai_family, node->ai_socktype, node->ai_protocol); // NOTE: Not bound to port yet
        if (chatter->serversock == -1) {
            printErrorGUI(gui, "Error on socket...trying another one\n");
            node = node->ai_next;
        }
        int yes = 1;
        if (setsockopt(chatter->serversock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            freeaddrinfo(info);
            socketErrorAndExit(chatter, "Error number %i setting socket options\n");
        }
        else if (bind(chatter->serversock, node->ai_addr, node->ai_addrlen) == -1) {
            freeaddrinfo(info);
            socketErrorAndExit(chatter, "Error number %i binding socket\n");
        }
    }
    freeaddrinfo(info);
    // Step 1c: Service requests (single threaded for now)
    if (chatter->serversock == -1 || node == NULL) {
        socketErrorAndExit(chatter, "ERROR: Error number %i on opening socket\n");
    }
    if (listen(chatter->serversock, BACKLOG) == -1) {
        socketErrorAndExit(chatter, "Error number %i listening on socket\n");
    }
    pthread_t serverThread;
    int res = pthread_create(&serverThread, NULL, serverLoop, (void*)chatter);
    if (res != 0) {
        socketErrorAndExit(chatter, "Error number %i creating server thread\n");
        destroyChatter(chatter);
        exit(res);
    }

    // Step 2: Begin the input loop on the client side
    typeLoop(chatter);

    // Step 3: Clean everything up when it's over
    destroyChatter(chatter);
}