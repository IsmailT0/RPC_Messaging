#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "messaging.h"

// Update ClientNode structure
typedef struct ClientNode {
    char client_name[100];
    int is_online;
    struct ClientNode *next;
} ClientNode;

// Structure to hold messages
typedef struct MessageNode {
    char sender[100];
    char recipient[100];
    char text[256];
    char timestamp[32];
    struct MessageNode *next;
} MessageNode;

// Global variables for the linked lists of clients and messages
ClientNode *clients = NULL;
MessageNode *messages = NULL;
MessageNode *messages_tail = NULL;  // Track the end of messages list

//register client 
int *register_client_1_svc(RegisterRequest *argp, struct svc_req *rqstp) {
    static int result;
    result = 0;

    ClientNode *cur = clients;
    while (cur != NULL) {
        if (strcmp(cur->client_name, argp->client_name) == 0) {
            if (cur->is_online) {
                printf("Client '%s' is already online.\n", argp->client_name);
                return &result;
            }
            cur->is_online = 1;
            printf("Client '%s' is back online.\n", argp->client_name);
            result = 1;
            return &result;
        }
        cur = cur->next;
    }

    ClientNode *new_client = (ClientNode *)malloc(sizeof(ClientNode));
    if (!new_client) {
        perror("Failed to allocate memory for new client");
        exit(1);
    }
    strcpy(new_client->client_name, argp->client_name);
    new_client->is_online = 1;
    new_client->next = clients;
    clients = new_client;

    printf("Client '%s' registered successfully.\n", argp->client_name);
    result = 1;
    return &result;
}

// Modify deregister_client_1_svc
int *deregister_client_1_svc(RegisterRequest *argp, struct svc_req *rqstp) {
    static int result;
    result = 0;

    ClientNode *cur = clients;
    while (cur != NULL) {
        if (strcmp(cur->client_name, argp->client_name) == 0) {
            cur->is_online = 0;
            printf("Client '%s' is now offline.\n", argp->client_name);
            result = 1;
            break;
        }
        cur = cur->next;
    }
    return &result;
}

/* Send a message */
int *send_message_1_svc(SendMessageRequest *argp, struct svc_req *rqstp) {
    static int result;
    result = 0; // Default to failure

    // Validate input parameters
    if (!argp || !argp->sender || !argp->recipient || !argp->text) {
        printf("Invalid message parameters\n");
        return &result;
    }

    // Check string lengths
    if (strlen(argp->sender) >= 100 || strlen(argp->recipient) >= 100 || strlen(argp->text) >= 256) {
        printf("Message content too long\n");
        result = -2;
        return &result;
    }

    // Check if the recipient exists
    ClientNode *cur = clients;
    int recipient_found = 0;
    while (cur != NULL) {
        if (strcmp(cur->client_name, argp->recipient) == 0) {
            recipient_found = 1;
            break;
        }
        cur = cur->next;
    }


    if (!recipient_found) {
        printf("Recipient '%s' not found.\n", argp->recipient);
        result = -1;
        return &result;
    }

    // Add the message to the linked list (at the end for chronological order)
    MessageNode *new_message = (MessageNode *)malloc(sizeof(MessageNode));
    if (!new_message) {
        perror("Failed to allocate memory for new message");
        return &result;
    }

    // Use strncpy for safe string copying
    strncpy(new_message->sender, argp->sender, sizeof(new_message->sender) - 1);
    new_message->sender[sizeof(new_message->sender) - 1] = '\0';
    
    strncpy(new_message->recipient, argp->recipient, sizeof(new_message->recipient) - 1);
    new_message->recipient[sizeof(new_message->recipient) - 1] = '\0';
    
    strncpy(new_message->text, argp->text, sizeof(new_message->text) - 1);
    new_message->text[sizeof(new_message->text) - 1] = '\0';

    // Get current timestamp
    time_t now;
    time(&now);
    strftime(new_message->timestamp, sizeof(new_message->timestamp),
             "%Y-%m-%d %H:%M:%S", localtime(&now));

    new_message->next = NULL;

    // Add to end of list
    if (messages == NULL) {
        messages = new_message;
        messages_tail = new_message;
    } else {
        messages_tail->next = new_message;
        messages_tail = new_message;
    }

    printf("Message from '%s' to '%s' queued successfully.\n", argp->sender, argp->recipient);

    result = 1; // Success
    return &result;
}




/* Fetch a message */
FetchMessageResponse *fetch_message_1_svc(FetchMessageRequest *argp, struct svc_req *rqstp) {
    static FetchMessageResponse result;
    static Message msg_array[100];
    int msg_count = 0;
    
    memset(&result, 0, sizeof(FetchMessageResponse));
    
    if (!argp || !argp->client_name) {
        result.messageArray.count = 0;
        return &result;
    }

    MessageNode *cur = messages;
    MessageNode *prev = NULL;
    MessageNode *next = NULL;
    
    // Collect messages and remove them
    while (cur != NULL) {
        next = cur->next;
        
        if (strcmp(cur->recipient, argp->client_name) == 0) {
            // Copy message to response array
            msg_array[msg_count].sender = strdup(cur->sender);
            msg_array[msg_count].recipient = strdup(cur->recipient);
            msg_array[msg_count].text = strdup(cur->text);
            msg_array[msg_count].timestamp = strdup(cur->timestamp);
            msg_count++;
            
            // Remove message from list
            if (prev == NULL) {
                messages = next;
            } else {
                prev->next = next;
            }
            
            // Update tail if needed
            if (cur == messages_tail) {
                messages_tail = prev;
            }
            
            free(cur);
            cur = next;
            continue;
        }
        
        prev = cur;
        cur = next;
    }
    
    result.messageArray.messages.messages_val = msg_array;
    result.messageArray.messages.messages_len = msg_count;
    result.messageArray.count = msg_count;
    
    printf("Fetched and removed %d messages for '%s'.\n", msg_count, argp->client_name);
    return &result;
}

