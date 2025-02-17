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
#include <netinet/tcp.h>
#include <time.h>
#include <sys/stat.h>

#include "linkedlist.h"
#include "hashmap.h"
#include "arraylist.h"
#include "chatter.h"

#define BACKLOG 20
#define ANNOUNCE_SENDING_FILE 1

///////////////////////////////////////////////////////////
//       Data Structure Memory Management
///////////////////////////////////////////////////////////

struct ReceiveData {
    struct Chatter* chatter;
    struct Chat* chat;
};

struct Chat* initChat(int sockfd) {
    debug_print("initChat called\n");
    struct Chat* chat = (struct Chat*)malloc(sizeof(struct Chat));
    chat->messagesIn = LinkedList_init();
    chat->messagesOut = LinkedList_init();
    chat->outCounter = 0;
    chat->sockfd = sockfd;
    return chat;
}

void destroyChat(struct Chat* chat) {
    debug_print("destroyChat called\n");
    freeMessages(chat->messagesIn);
    LinkedList_free(chat->messagesIn);
    freeMessages(chat->messagesOut);
    LinkedList_free(chat->messagesOut);
    free(chat);
}

void freeMessages(struct LinkedList* messages) {
    debug_print("freeMessages called\n");
    struct LinkedNode* node = messages->head;
    while (node != NULL) {
        struct Message* message = (struct Message*)node->data;
        free(message->text); // This assumes the message text has been dynamically allocated
        free(message);
        node = node->next;
    }
}

struct Chatter* initChatter() {
    debug_print("initChatter called\n");
    // Dynamically allocate all objects that need allocating
    struct Chatter* chatter = (struct Chatter*)malloc(sizeof(struct Chatter));
    chatter->gui = initGUI();
    strcpy(chatter->myname, "Anonymous");
    chatter->chats = LinkedList_init();
    chatter->visibleChat = NULL;
    pthread_mutex_init(&chatter->lock, NULL);
    /////////////////////////////////////////
    // Refresh the GUI thread every so often
    int res = pthread_create(&chatter->refreshGUIThread, NULL, refreshGUILoop, (void*)chatter);
    if (res != 0) {
        fprintf(stderr, "Error setting up refresh daemon for GUI\n");
    }
    /////////////////////////////////////////
    return chatter;
}

void destroyChatter(struct Chatter* chatter) {
    debug_print("destroyChatter called\n");

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
    debug_print("getChatFromName called\n");

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

int _send_loop(int sockfd,char *src,size_t len){
    int status = STATUS_SUCCESS;
    ssize_t sent_bytes;

    while (len > 0){
        sent_bytes = send(sockfd,src,len,0);
        if(sent_bytes == -1){
            status = FAILURE_GENERIC;
            break;
        }
        src += sent_bytes;
        len -= sent_bytes;
    }

    return status;
}

int _recv_loop(int sockfd,char *dst,size_t len){
    int status = STATUS_SUCCESS;
    ssize_t res;

    while(len > 0){
        res = recv(sockfd,dst,len,0);
        if (res <= 0) {
            perror("recv");
            debug_print("res: %ld\n",res);
            status = FAILURE_GENERIC;
            break;
        }
        dst += res;
        len -= res;
    }

    return status;
}

/**
 * @brief Remove a particular chat from the list
 * 
 * @param chatter Chatter object
 * @param chat Chat to remove
 */
void removeChat(struct Chatter* chatter, struct Chat* chat) {
    pthread_mutex_lock(&chatter->lock);
    debug_print("REMOVE CHAT WAS CALLED!!!!\n");
    LinkedList_remove(chatter->chats, chat);
    if (chatter->visibleChat == chat) {
        // Bounce to another chat if there is one
        chatter->visibleChat = NULL;
        if (chatter->chats->head != NULL) {
            chatter->visibleChat = (struct Chat*)chatter->chats->head->data;
        }
    }
    destroyChat(chat);
    pthread_mutex_unlock(&chatter->lock);
}


///////////////////////////////////////////////////////////
//             Chat Session Messages In
///////////////////////////////////////////////////////////

/**
 * @brief Continually loop and receive info on a particular chat
 * 
 * @param pargs 
 * @return void* 
 */
void* receiveLoop(void* pargs) {
    struct ReceiveData* param = (struct ReceiveData*)pargs;
    struct Chat* chat = param->chat;
    struct Chatter* chatter = param->chatter;
    free(param);

    int continue_server_loop = 1, status = STATUS_SUCCESS, res;
    char *msg_text, *filename, buf[1024];
    struct Message *msg_obj;
    uint16_t incoming_short;
    uint32_t incoming_long;
    size_t bytes_written;
    FILE *file;

    while(continue_server_loop){
        // Loop until the connection closes
        struct header_generic msg_header;
        
        status = _recv_loop(chat->sockfd,(char*)&msg_header,sizeof(struct header_generic));
        if (status != STATUS_SUCCESS) {
            debug_print("RECV FAILURE!!\n");
            break;
        }

        // Be sure to lock variables as appropriate for thread safety
        switch(msg_header.magic){
            case INDICATE_NAME:
                debug_print("NAME recvd\n");

                incoming_short = ntohs(msg_header.shortInt);

                status = _recv_loop(chat->sockfd,chat->name,incoming_short);
                chat->name[incoming_short] = '\0';

                if(status != STATUS_SUCCESS){
                    continue_server_loop = 0;
                }
                break;

            case SEND_MESSAGE:
                debug_print("MESSAGE recvd\n");

                msg_obj = malloc(sizeof(struct Message));
                incoming_short = ntohs(msg_header.shortInt);
                incoming_long = ntohl(msg_header.longInt);
                msg_text = malloc((uint64_t)incoming_long+1);

                status = _recv_loop(chat->sockfd,msg_text,incoming_long);
                msg_text[incoming_long] = '\0';
                if(status != STATUS_SUCCESS){
                    continue_server_loop = 0;
                }

                msg_obj->id = incoming_short;
                msg_obj->timestamp = time(NULL);
                msg_obj->text = msg_text;
                LinkedList_addFirst(chat->messagesIn,msg_obj);
                break;

            case DELETE_MESSAGE:
                debug_print("DELETE NAME recvd\n");
                
                incoming_short = ntohs(msg_header.shortInt);
                deleteMessageFromChat(chat,incoming_short);
                break;

            case SEND_FILE:
                debug_print("FILE recvd\n");

                incoming_short = ntohs(msg_header.shortInt);
                incoming_long = ntohs(msg_header.shortInt);
                filename = malloc((uint32_t)incoming_short+1);

                status = _recv_loop(chat->sockfd,filename,incoming_short);
                filename[incoming_short] = '\0';
                if(status != STATUS_SUCCESS){
                    continue_server_loop = 0;
                }

                file = fopen(filename,"rb");
                free(filename);
                if(file == NULL){
                    continue_server_loop = 0;
                }
                else{
                    while(incoming_long > 0){
                        res = recv(chat->sockfd,buf,incoming_long<1024 ? incoming_long : 1024,0);
                        if (res <= 0) {
                            continue_server_loop = 0;
                            break;
                        }
                        bytes_written = fwrite(buf,sizeof(char),res,file);
                        if (bytes_written < res) {
                            continue_server_loop = 0;
                            break;
                        }
                        incoming_long -= res;
                    }
                    fclose(file);
                }

                break;

            case END_CHAT:
                debug_print("END CHAT recvd\n");

                removeChat(chatter,chat);
                break;

            default:
                debug_print("Unknown magic number %d received",msg_header.magic);
                break;
        }
        reprintUsernameWindow(chatter);
        reprintChatWindow(chatter);
    }
    debug_print("ENDING RECEIVE LOOP!\n");
    removeChat(chatter, chat);
    pthread_exit(NULL); // Clean up thread
    return NULL;
}



///////////////////////////////////////////////////////////
//             Chat Session Messages Out
///////////////////////////////////////////////////////////


/**
 * @brief Send a message in the visible chat
 * 
 * @param chatter Data about the current chat session
 * @param message 
 */
int sendMessage(struct Chatter* chatter, char* message) {
    int status = STATUS_SUCCESS;
    pthread_mutex_lock(&chatter->lock);
    
    if (chatter->visibleChat == NULL){
        status = FAILURE_GENERIC;
    }
    else{
        // Handle adding the message locally
        struct Message *msg_obj = malloc(sizeof(struct Message));
        uint16_t msg_id = chatter->visibleChat->outCounter++;
        uint32_t remaining_len = strlen(message);
        char *msg_text = malloc((uint64_t)remaining_len+1);
        strcpy(msg_text,message);
        msg_obj->id = msg_id;
        msg_obj->timestamp = time(NULL);
        msg_obj->text = msg_text;
        LinkedList_addFirst(chatter->visibleChat->messagesOut,msg_obj);

        // Handle sending message
        struct header_generic msg_header;
        msg_header.magic = SEND_MESSAGE;
        msg_header.shortInt = htons(msg_id);
        msg_header.longInt = htonl(remaining_len);
        
        status = _send_loop(chatter->visibleChat->sockfd,(char*)&msg_header,sizeof(struct header_generic));
        if(status == STATUS_SUCCESS){
            status = _send_loop(chatter->visibleChat->sockfd,message,remaining_len);
        }
    }
    
    pthread_mutex_unlock(&chatter->lock);
    return status;
}

/**
 * @brief Delete message in the visible chat
 * 
 * @param chatter Data about the current chat session
 * @param id ID of message to delete
 */
int deleteMessage(struct Chatter* chatter, uint16_t id) {
    pthread_mutex_lock(&chatter->lock);

    // Locally remove the message
    int status = deleteMessageFromChat(chatter->visibleChat,id);

    // Send to remove the message on the remote connection
    struct header_generic msg_header;
    msg_header.magic = DELETE_MESSAGE;
    msg_header.shortInt = htons(id);

    status = _send_loop(chatter->visibleChat->sockfd,(char*)&msg_header,sizeof(struct header_generic));

    pthread_mutex_unlock(&chatter->lock);
    return status;
}

int deleteMessageFromChat(struct Chat *chat, uint16_t id){
    int status = FAILURE_GENERIC;
    struct LinkedList *messagesOut = chat->messagesOut;
    for(struct LinkedNode *node = messagesOut->head; node != NULL; node=node->next){
        struct Message *message = (struct Message*)node->data;
        if(message->id == id){
            free(message->text);
            LinkedList_remove(messagesOut,message);
            free(message);
            status = STATUS_SUCCESS;
            break;
        }
    }
    return status;
}

/**
 * @brief Send a file in the visible chat
 * 
 * @param chatter Data about the current chat session
 * @param filename Path to file
 */
int sendFile(struct Chatter* chatter, char* filename) {
    int status = STATUS_SUCCESS;
    if(ANNOUNCE_SENDING_FILE){
        char *announce_msg = malloc(strlen(filename)+1+17);
        sprintf(announce_msg,"(Sending file '%s')",filename);
        sendMessage(chatter,announce_msg);
        free(announce_msg);
    }
    pthread_mutex_lock(&chatter->lock);

    struct stat file_stat;
    if(stat(filename,&file_stat) == -1){
        status = FAILURE_GENERIC;
    }
    else{
        uint16_t remaining_fn_length = strlen(filename);
        uint32_t remaining_file_length = file_stat.st_size;
        struct header_generic msg_header;
        msg_header.magic = SEND_FILE;
        msg_header.shortInt = htons(remaining_fn_length);
        msg_header.longInt = htonl(remaining_file_length);

        FILE *file = fopen(filename,"rb");
        if(file == NULL){
            status = FAILURE_GENERIC;
        }
        else{
            status = _send_loop(chatter->visibleChat->sockfd,(char*)&msg_header,sizeof(struct header_generic));
            if(status == STATUS_SUCCESS){
                ssize_t sent_bytes;
                size_t remaining_in_buffer = 0;

                status = _send_loop(chatter->visibleChat->sockfd,filename,remaining_fn_length);

                char buf[1024];
                while(remaining_file_length > 0){
                    if(remaining_in_buffer == 0){
                        remaining_in_buffer = fread(buf,sizeof(char),1024,file);
                        if(remaining_in_buffer < 1024 && ferror(file)){
                            status = FAILURE_GENERIC;
                            break;
                        }
                    }
                    sent_bytes = send(chatter->visibleChat->sockfd,buf,remaining_in_buffer,0);
                    if(sent_bytes == -1){
                        status = FAILURE_GENERIC;
                        break;
                    }
                    remaining_in_buffer -= sent_bytes;
                    remaining_file_length -= sent_bytes;
                    if(remaining_in_buffer > 0){
                        memmove(buf,buf+sent_bytes,remaining_in_buffer);
                    }
                }
            }
            
            fclose(file);
        }
    }

    pthread_mutex_unlock(&chatter->lock);
    return status;
}

/**
 * @brief Broadcast my name to all visible connections
 * NOTE: Name is held in chatter->myname
 * 
 * @param chatter Data about the current chat session
 */
int broadcastMyName(struct Chatter* chatter) {
    int status = STATUS_SUCCESS;
    pthread_mutex_lock(&chatter->lock);

    debug_print("Hello from broadcast myname!\n");
    debug_print("Broadcast name, chatter*: %p\n",(void*)chatter);
    
    size_t remaining_len = strlen(chatter->myname);
    struct header_generic msg_header;
    msg_header.magic = INDICATE_NAME;
    msg_header.shortInt = htons((uint16_t)remaining_len);

    debug_print("Hello from after header creation!\n");

    for(struct LinkedNode *curr_node = chatter->chats->head; curr_node != NULL; curr_node = curr_node->next){
        debug_print("Hello from inside the loop!\n");
        struct Chat *curr_chat = (struct Chat*)curr_node->data;
        if(send(curr_chat->sockfd,(char*)&msg_header,sizeof(struct header_generic),0) == -1){
            status = FAILURE_GENERIC;
        }
        else{
            char *message = chatter->myname;
            ssize_t sent_bytes;
            while (remaining_len > 0){
                sent_bytes = send(curr_chat->sockfd,message,remaining_len,0);
                if(sent_bytes == -1){
                    status = FAILURE_GENERIC;
                    break;
                }
                message += sent_bytes;
                remaining_len -= sent_bytes;
            }
        }
    }

    pthread_mutex_unlock(&chatter->lock);
    debug_print("Hello from after the unlock!\n");
    return status;
}

/**
 * @brief Close chat with someone
 * 
 * @param chatter Data about the current chat session
 * @param name Close connection with this person
 */
int closeChat(struct Chatter* chatter, char* name) {
    debug_print("closeChat called\n");

    int status = STATUS_SUCCESS;
    
    struct Chat *selected_chat = getChatFromName(chatter,name);

    struct header_generic msg_header;
    msg_header.magic = END_CHAT;

    if(send(selected_chat->sockfd,(char*)&msg_header,sizeof(struct header_generic),0) == -1){
        status = FAILURE_GENERIC;
    }
    removeChat(chatter,selected_chat);

    return status;
}


/**
 * @brief Switch the visible chat
 * 
 * @param chatter Data about the current chat session
 * @param name Switch chat to be with this person
 */
int switchTo(struct Chatter* chatter, char* name) {
    int status = STATUS_SUCCESS;
    struct Chat* chat = getChatFromName(chatter, name);
    pthread_mutex_lock(&chatter->lock);
    if (chat == NULL) {
        status = CHAT_DOESNT_EXIST;
    }
    else {
        chatter->visibleChat = chat;
    }
    pthread_mutex_unlock(&chatter->lock);
    return status;
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
int setupNewChat(struct Chatter* chatter, int sockfd) {
    // Step 0: Disable Nagle's algorithm on this socket
    int yes = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int));
    // Step 1: Setup a new chat object and add to the list
    struct Chat* chat = initChat(sockfd);
    strcpy(chat->name, "Anonymous");
    pthread_mutex_lock(&chatter->lock);
    LinkedList_addFirst(chatter->chats, (void*)chat);
    debug_print("In setup new chat, chat list head: %p\n",(void*)chatter->chats->head);
    debug_print("In setup new chat, sockfd: %d\n",sockfd);
    // Step 2: Start a thread for a loop that receives data
    struct ReceiveData* param = (struct ReceiveData*)malloc(sizeof(struct ReceiveData));
    param->chat = chat;
    param->chatter = chatter;
    pthread_t receiveThread;
    int res = pthread_create(&receiveThread, NULL, receiveLoop, (void*)param);
    int status = STATUS_SUCCESS;
    if (res != 0) {
        // Print out error information
        char* fmt = "Error %i opening new connection";
        char* error = (char*)malloc(strlen(fmt) + 100);
        sprintf(error, fmt, errno);
        printErrorGUI(chatter->gui, error);
        free(error);
        // Remove dynamically allocated stuff
        LinkedList_removeFirst(chatter->chats);
        status = ERR_THREADCREATE;
    }
    else if (chatter->chats->head->next == NULL) {
        // This is the first chat; make it visible
        chatter->visibleChat = chat;
    }
    pthread_mutex_unlock(&chatter->lock);
    if (status != STATUS_SUCCESS) {
        debug_print("ISSUE OCCURRED IN SETUP, DESTROYING CHAT!!");
        destroyChat(chat);
    }
    debug_print("Setup new chat about to return, chat list head: %p\n",(void*)chatter->chats->head);
    debug_print("Setup new chat, chatter*: %p\n",(void*)chatter);
    return status;
}


/**
 * @brief Establish a chat as a client connecting to an IP/port
 * 
 * @param chatter Data about the current chat session
 * @param IP IP address in human readable form
 * @param port Port on which to establish connection
 */
int connectChat(struct Chatter* chatter, char* IP, char* port) {
    // Step 1: Get address information for a host
    struct addrinfo hints;
    struct addrinfo* node;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // Use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP
    int ret = getaddrinfo(IP, port, &hints, &node);
    if (ret != 0) {
        printErrorGUI(chatter->gui, "Error getting address info");
        freeaddrinfo(node);
        return ERR_GETADDRINFO;
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
        return ERR_OPENSOCKET;
    }
    // Step 2: Setup stream on socket and connect
    ret = connect(sockfd, node->ai_addr, node->ai_addrlen);
    freeaddrinfo(node);
    if (ret == -1) {
        printErrorGUI(chatter->gui, "Error opening socket");
        return ERR_OPENSOCKET;
    }
    ret = setupNewChat(chatter, sockfd);
    debug_print("in connectChat before return... chats*: %p, chats head: %p\n",(void*)chatter->chats,(void*)chatter->chats->head);
    return ret;
}

/**
 * @brief Continually loop through and accept new connections
 * 
 * @param pargs Pointer to the chatter data
 */
void* serverLoop(void* pargs) {
    struct Chatter* chatter = (struct Chatter*)pargs;
    while (1) { // TODO: Finish terminating thread when appropriate
        struct sockaddr_storage their_addr;
        socklen_t len = sizeof(their_addr);
        int sockfd = accept(chatter->serversock, (struct sockaddr*)&their_addr, &len);
        if (sockfd != -1) {
            int result = setupNewChat(chatter, sockfd);
            if (result != STATUS_SUCCESS) {
                printErrorGUI(chatter->gui, "Error receiving new connection");
            }
            else {
                reprintUsernameWindow(chatter);
                reprintChatWindow(chatter);
            }
        }
        else {
            printErrorGUI(chatter->gui, "Error receiving new connection");
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
    // TODO: Cleanup server thread

    // Step 3: Clean everything up when it's over
    destroyChatter(chatter);
}