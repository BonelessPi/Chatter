#ifndef CHATTER_H
#define CHATTER_H

#include <stdint.h>
#include <ncurses.h>
#include <pthread.h>
#include "linkedlist.h"
#include "hashmap.h"

#define DEBUG 1
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)


enum ChatStatusCodes {
    STATUS_SUCCESS = 0,
    FAILURE_GENERIC = 1,
    IP_FORMAT_ERROR = 2,
    KEEP_GOING = 3,
    READY_TO_EXIT = 4,
    CHAT_DOESNT_EXIST = 5,
    ERR_GETADDRINFO = 6,
    ERR_OPENSOCKET = 7,
    ERR_THREADCREATE = 8
};

enum Magic {
    INDICATE_NAME = 0,
    SEND_MESSAGE = 1,
    DELETE_MESSAGE = 2,
    SEND_FILE = 3,
    END_CHAT = 4
};

struct __attribute__((__packed__))  header_generic {
    uint8_t magic;
    uint16_t shortInt; // Because @bonelesspi said so
    uint32_t longInt; // Because @thekacefiles said it was too archaic
};

struct GUI {
    int W, H; // Width, height of terminal
    int CH; // Chat height
    int CW; // Chat width
    WINDOW* chatWindow;
    pthread_mutex_t chatWindowLock;
    WINDOW* inputWindow;
    WINDOW* nameWindow;
    pthread_mutex_t nameWindowLock;
};

struct Message {
    uint16_t id;
    time_t timestamp; // Time at which this message was added to the data structure
    char* text;
};

struct Chat {
    char name[65536]; // Name of the person we're talking to
    int sockfd; // Socket associated to this chat
    uint16_t outCounter; // How many messages sent out on this chat
    struct LinkedList* messagesIn;
    struct LinkedList* messagesOut;
};

struct Chatter {
    struct GUI* gui;
    struct LinkedList* chats;
    char myname[65536];
    struct Chat* visibleChat; // Linked node for the visible chat
    int serversock; // File descriptor for the socket listening for incoming connections
    pthread_mutex_t lock;
};


// GUI functions

struct GUI* _init_GUI();
void _free_GUI(struct GUI* gui);
void printErrorGUI(struct GUI* gui, char* error);

void reprintUsernameWindow(struct Chatter* chatter); // NOTE: This method locks chat
void printLineToChat(struct GUI* gui, char* str, int len, int* row);
void reprintChatWindow(struct Chatter* chatter); // NOTE: This method locks chat

int parseInput(struct Chatter* chatter, char* input);
void typeLoop(struct Chatter* chatter);

// Chatter base functions

struct Chatter* _init_chatter();
void _free_chatter(struct Chatter* chatter);
struct Chat* _init_chat(int sockfd);
void _free_chat(struct Chat* chat);
void _free_message_list(struct LinkedList* messages);

int _send_loop(int sockfd,char *src,size_t len);
int _recv_loop(int sockfd,char *dst,size_t len);

// Helper functions

struct Chat* getChatFromName(struct Chatter* chatter, char* name);
void removeChat(struct Chatter *chatter, struct Chat *chat);

// Receiving task

void* receiveLoop(void *pargs);

// Action functions

/**
 * @brief Send a message in the visible chat
 * 
 * @param chatter Data about the current chat session
 * @param message 
 */
int sendMessage(struct Chatter* chatter, char* message);

/**
 * @brief Delete message in the visible chat
 * 
 * @param chatter Data about the current chat session
 * @param id ID of message to delete
 */
int deleteMessage(struct Chatter* chatter, uint16_t id);

int _delete_message_from_list(struct LinkedList *list, uint16_t id);

/**
 * @brief Send a file in the visible chat
 * 
 * @param chatter Data about the current chat session
 * @param filename Path to file
 */
int sendFile(struct Chatter* chatter, char* filename);

/**
 * @brief Broadcast my name to all visible connections
 * NOTE: Name is held in chatter->myname
 * 
 * @param chatter Data about the current chat session
 */
int broadcastMyName(struct Chatter* chatter);

int _declare_name_to_chat(struct Chat *chat, char *name, size_t len);

/**
 * @brief Close chat with someone
 * 
 * @param chatter Data about the current chat session
 * @param name Close connection with this person
 */
int closeChat(struct Chatter* chatter, char* name);

/**
 * @brief Switch the visible chat
 * 
 * @param chatter Data about the current chat session
 * @param name Switch chat to be with this person
 */
int switchTo(struct Chatter* chatter, char* name);

// Connection Management

void socketErrorAndExit(struct Chatter* chatter, char* fmt);
int setupNewChat(struct Chatter* chatter, int sockfd);

/**
 * @brief Establish a chat with an IP address
 * 
 * @param chatter Data about the current chat session
 * @param IP IP address in human readable form
 * @param port Port on which to establish connection
 */
int connectChat(struct Chatter* chatter, char* IP, char* port);

// Task for server function

void* serverLoop(void* pargs);


#endif
