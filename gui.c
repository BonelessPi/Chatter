// Programmer: Chris Tralie
// Purpose: To provide a simple 3 window interface where
// users can type commands and manage chats

#include "chatter.h"
#include "arraylist.h"
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define TYPE_SIZE 4
#define ADDR_WIDTH 10
char NULLTERM = '\0';

struct GUI* initGUI() {
    // Setup 3 windows
    initscr();
    noecho();
    keypad(stdscr, TRUE);
    struct GUI* gui = (struct GUI*)malloc(sizeof(struct GUI));
    getmaxyx(stdscr, gui->H, gui->W);
    gui->CH = gui->H - TYPE_SIZE;
    gui->CW = gui->W - ADDR_WIDTH;
    curs_set(TRUE);
    gui->chatWindow = newwin(gui->CH, gui->CW, 0, 0);
    //pthread_mutex_init(&gui->chatWindowLock, NULL);
    gui->inputWindow = newwin(TYPE_SIZE, gui->W, gui->CH, 0);
    gui->nameWindow  = newwin(gui->CH, ADDR_WIDTH, 0, gui->CW);
    //pthread_mutex_init(&gui->nameWindowLock, NULL);

    char* s = "Hello!  Chats will go here!";
    mvwprintw(gui->chatWindow, 0, 0, "%s", s); 
    wrefresh(gui->chatWindow);
    mvwprintw(gui->nameWindow, 0, 0, "Usernames\nGo\nHere!"); 
    wrefresh(gui->nameWindow);
    return gui;
}

void destroyGUI(struct GUI* gui) {
    delwin(gui->chatWindow);
    pthread_mutex_destroy(&gui->chatWindowLock);
    delwin(gui->inputWindow);
    delwin(gui->nameWindow);
    pthread_mutex_destroy(&gui->nameWindowLock);
    endwin();
    free(gui);
}



///////////////////////////////////////////////////////////
//       GUI Update Methods
//       See: https://linux.die.net/man/3/mvprintw
///////////////////////////////////////////////////////////

void printErrorGUI(struct GUI* gui, char* error) {
    //pthread_mutex_lock(&gui->chatWindowLock);
    wclear(gui->chatWindow);
    mvwprintw(gui->chatWindow, 0, 0, "%s", error);
    wrefresh(gui->chatWindow);
    //pthread_mutex_unlock(&gui->chatWindowLock);
}

void reprintUsernameWindow(struct Chatter* chatter) {
    struct GUI* gui = chatter->gui;
    pthread_mutex_lock(&chatter->lock);
    //pthread_mutex_lock(&gui->nameWindowLock);
    wclear(gui->nameWindow);
    struct LinkedNode* node = chatter->chats->head;
    int row = 0;
    while (node != NULL && row < gui->CH) {
        struct Chat* chat = (struct Chat*)node->data;
        char special = ' ';
        if (chat == chatter->visibleChat) {
            special = '*'; // Put an asterix next to the active chat
        }
        mvwprintw(gui->nameWindow, row, 0, "%s%c", chat->name, special);
        row++;
        node = node->next;
    }
    wrefresh(gui->nameWindow);
    //pthread_mutex_unlock(&gui->nameWindowLock);
    pthread_mutex_unlock(&chatter->lock);
}

void printLineToChat(struct GUI* gui, char* str, int len, int* row) {
    int col = 0;
    for (int i = 0; i < len; i++) {
        mvwprintw(gui->chatWindow, *row, col, "%c", str[i]);
        col++;
        if (col == gui->W) {
            col = 0;
            (*row)--;
        }
    }
}

void reprintChatWindow(struct Chatter* chatter) {
    struct GUI* gui = chatter->gui;
    pthread_mutex_lock(&chatter->lock);
    //pthread_mutex_lock(&gui->chatWindowLock);
    wclear(gui->chatWindow);
    if (chatter->visibleChat != NULL) {
        struct Chat* chat = chatter->visibleChat;
        // Print out chats in order of most recent
        int row = gui->CH - 1;
        struct LinkedNode* inNode = chat->messagesIn->head;
        struct LinkedNode* outNode = chat->messagesOut->head;
        while (row >= 0 && (inNode != NULL || outNode != NULL)) {
            int printOut = 0;
            if (inNode == NULL) {
                printOut = 1;
            }
            else if (outNode != NULL) {
                struct Message* msgIn = (struct Message*)inNode->data;
                struct Message* msgOut = (struct Message*)outNode->data;
                if (msgOut->timestamp > msgIn->timestamp) {
                    printOut = 1;
                }
            }
            if (printOut == 1) {
                // Print my message
                struct Message* msg = (struct Message*)outNode->data;
                char* str = (char*)malloc(strlen("Me : ") + strlen(msg->text) + 100);
                sprintf(str, "Me %i: %s", msg->id, msg->text);
                printLineToChat(gui, str, strlen(str), &row);
                free(str);
                outNode = outNode->next;
            }
            else {
                // Print other message
                struct Message* msg = (struct Message*)inNode->data;
                char* str = (char*)malloc(strlen(chat->name) + strlen(msg->text) + 100);
                sprintf(str, "%s %i: %s", chat->name, msg->id, msg->text);
                printLineToChat(gui, str, strlen(str), &row);
                free(str);
                inNode = inNode->next;
            }
            row--;
        }
    }
    wrefresh(gui->chatWindow);
    //pthread_mutex_unlock(&gui->chatWindowLock);
    pthread_mutex_unlock(&chatter->lock);
}

void* refreshGUILoop(void* args) {
    struct Chatter* chatter = (struct Chatter*)args;
    while (1) {
        sleep(5);
        reprintUsernameWindow(chatter);
        reprintChatWindow(chatter);
    }
}

///////////////////////////////////////////////////////////
//            Typing / Parsing Methods
///////////////////////////////////////////////////////////


/**
 * @brief Incorporate something that the user typed into
 * the chat session
 * 
 * @param chatter Chat session object
 * @param input String that the user just inputted
 */
int parseInput(struct Chatter* chatter, char* input) {
    int finishedStatus = KEEP_GOING;
    int status = 0;
    struct GUI* gui = chatter->gui;
    // "connect <IP>"
    if (strncmp(input, "connect", strlen("connect")) == 0) {
        // Connect and start a conversation with a particular IP address
        char IP[40];
        char port[6];
        sscanf(input, "connect %39s %5s", IP, port);
        if (strstr(IP, ".") == NULL) {
            status = IP_FORMAT_ERROR;
            printErrorGUI(gui, "Please put a dot in your IP address!");
        }
        else {
            status = connectChat(chatter, IP, port);
        }
    }
    else if (strncmp(input, "myname", strlen("myname")) == 0) {
        // Change my name
        sscanf(input, "myname %65535s", chatter->myname);
        status = broadcastMyName(chatter);
    }
    else if (strncmp(input, "sendfile", strlen("sendfile")) == 0) {
        // Send the following file message in the active conversation
        char filename[65536];
        sscanf(input, "sendfile %s", filename);
        status = sendFile(chatter, filename);
    }
    else if (strncmp(input, "send", strlen("send")) == 0) {
        // Send the following text message in the active conversation
        char* message = input + strlen("send") + 1;
        status = sendMessage(chatter, message);
    }
    else if (strncmp(input, "talkto", strlen("talkto")) == 0) {
        // Switch the active chat window to someone else
        char name[65536];
        sscanf(input, "talkto %65535s", name);
        status = switchTo(chatter, name);
    }
    else if (strncmp(input, "delete", strlen("delete")) == 0) {
        // Delete the message with this id in the active conversation
        uint16_t id;
        sscanf(input, "delete %hu", &id);
        status = deleteMessage(chatter, id);
    }
    else if (strncmp(input, "close", strlen("close")) == 0) {
        // Close the connection with someone
        char name[65536];
        sscanf(input, "close %65535s", name);
        status = closeChat(chatter, name);
    }
    else if (strncmp(input, "exit", strlen("exit")) == 0) {
        finishedStatus = READY_TO_EXIT;
    }
    else {
        char* fmt = "Unrecognized command %s;  (use connect, myname, send, sendfile, delete, close, talkto, exit)";
        char command[65536];
        sscanf(input, "%65535s", command);
        char* error = (char*)malloc(strlen(fmt) + strlen(command) + 1);
        sprintf(error, fmt, command);
        printErrorGUI(gui, error);
        free(error);
        return finishedStatus;
    }
    if (status == STATUS_SUCCESS) {
        reprintUsernameWindow(chatter);
        reprintChatWindow(chatter);
    }

    return finishedStatus;
}

void typeLoop(struct Chatter* chatter) {
    struct GUI* gui = chatter->gui;
    ArrayListBuf buf;
    int finishedStatus = KEEP_GOING;
    while (finishedStatus == KEEP_GOING) {
        wclear(gui->inputWindow);
        // Input loop
        ArrayListBuf_init(&buf);
        int ch = 0;
        do {
            ch = wgetch(gui->inputWindow);
            if (ch == KEY_UP) {
                // TODO: Scroll up in current conversation
            }
            else if (ch == KEY_DOWN) {
                // TODO: Scroll down in current conversation
            }
            else if (ch == KEY_BACKSPACE || ch == KEY_DC || ch == 127) {
                // Handle deletes in the buffer
                if (buf.N > 0) {
                    buf.buff[buf.N-1] = '\0';
                    buf.N--;
                }
            }
            else if(ch != '\n') {
                // Accumulate current character
                ArrayListBuf_push(&buf, (char*)&ch, 1);
            }
            wclear(gui->inputWindow);
            for (int i = 0; i < buf.N; i++) {
                mvwprintw(gui->inputWindow, i/gui->W, i%gui->W, "%c", buf.buff[i]);
            }
        }
        while (ch != '\n');
        ArrayListBuf_push(&buf, &NULLTERM, 1);
        char* input = buf.buff;
        finishedStatus = parseInput(chatter, input);
        ArrayListBuf_free(&buf);
    }
}
