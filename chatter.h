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
struct GUI* initGUI();
void destroyGUI(struct GUI* gui);
void printErrorGUI(struct GUI* gui, char* error);

struct Message {
    uint16_t id;
    time_t timestamp; // Time at which this message was added to the data structure
    char* text;
};
void freeMessages(struct LinkedList* messages);

struct Chat {
    char name[65536]; // Name of the person we're talking to
    int sockfd; // Socket associated to this chat
    uint16_t outCounter; // How many messages sent out on this chat
    struct LinkedList* messagesIn;
    struct LinkedList* messagesOut;
};
struct Chat* initChat(int sockfd);
void destroyChat(struct Chat* chat);
void* refreshGUILoop(void* args);

struct Chatter {
    struct GUI* gui;
    struct LinkedList* chats;
    char myname[65536];
    struct Chat* visibleChat; // Linked node for the visible chat
    int serversock; // File descriptor for the socket listening for incoming connections
    pthread_mutex_t lock;
    pthread_t refreshGUIThread;
};
struct Chatter* initChatter();
void destroyChatter(struct Chatter* chatter);
struct Chat* getChatFromName(struct Chatter* chatter, char* name);

void reprintUsernameWindow(struct Chatter* chatter); // NOTE: This method locks chat
void reprintChatWindow(struct Chatter* chatter); // NOTE: This method locks chat
void typeLoop(struct Chatter* chatter);


////////////////////////////////////////////////////////////////////
//                    OUTGOING ACTIONS                            //
////////////////////////////////////////////////////////////////////

/**
 * @brief Establish a chat with an IP address
 * 
 * @param chatter Data about the current chat session
 * @param IP IP address in human readable form
 * @param port Port on which to establish connection
 */
int connectChat(struct Chatter* chatter, char* IP, char* port);

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

int deleteMessageFromChat(struct Chat *chat, uint16_t id);

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


////////////////////////////////////////////////////////////////////
//                    INCOMING ACTIONS                            //
////////////////////////////////////////////////////////////////////


#endif
