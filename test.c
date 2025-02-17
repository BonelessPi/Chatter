#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main() {
    // TODO: This is a dummy file for testing C syntax ideas 
    char* input = "send This is a message";
    if (strncmp(input, "send", strlen("send")) == 0) {
        char* message = input + strlen("send") + 1;
        printf("Sending message %s", message);

    }

}