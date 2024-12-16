#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "messaging.h"

// Structure to hold registered clients
typedef struct ClientNode {
    char client_name[100];
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

/* Register a client */
int *register_client_1_svc(RegisterRequest *argp, struct svc_req *rqstp) {
    static int result;
    result = 0; // Default to failure

    // Check if the client name is already registered
    ClientNode *cur = clients;
    while (cur != NULL) {
        if (strcmp(cur->client_name, argp->client_name) == 0) {
            result = -1; // Client name already exists
            return &result;
        }
        cur = cur->next;
    }

    // Add the new client to the linked list
    ClientNode *new_client = (ClientNode *)malloc(sizeof(ClientNode));
    if (!new_client) {
        perror("Failed to allocate memory for new client");
        exit(1);
    }
    strcpy(new_client->client_name, argp->client_name);
    new_client->next = clients;
    clients = new_client;

    printf("Client '%s' registered successfully.\n", argp->client_name);

    result = 1; // Success
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
    static Message msg_array[100];  // Assuming max 100 messages
    int msg_count = 0;
    
    // Clear previous response
    memset(&result, 0, sizeof(FetchMessageResponse));
    
    if (!argp || !argp->client_name) {
        result.messageArray.count = 0;
        return &result;
    }

    MessageNode *cur = messages;
    MessageNode *prev = NULL;
    MessageNode *temp = NULL;
    
    // Create temporary array to store messages in correct order
    MessageNode *temp_messages[100];
    msg_count = 0;
    
    // First pass: collect matching messages
    while (cur != NULL) {
        if (strcmp(cur->recipient, argp->client_name) == 0) {
            temp_messages[msg_count++] = cur;
        }
        cur = cur->next;
    }
    
    // Copy messages in correct order
    for (int i = 0; i < msg_count; i++) {
        msg_array[i].sender = strdup(temp_messages[i]->sender);
        msg_array[i].recipient = strdup(temp_messages[i]->recipient);
        msg_array[i].text = strdup(temp_messages[i]->text);
        msg_array[i].timestamp = strdup(temp_messages[i]->timestamp);
    }
    
    // Remove the messages from the list
    cur = messages;
    prev = NULL;
    while (cur != NULL) {
        if (strcmp(cur->recipient, argp->client_name) == 0) {
            MessageNode *to_free = cur;
            if (prev == NULL) {
                messages = cur->next;
                cur = messages;
            } else {
                prev->next = cur->next;
                cur = cur->next;
            }
            if (to_free == messages_tail) {
                messages_tail = prev;
            }
            free(to_free);
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
    
    // Set response
    result.messageArray.messages.messages_val = msg_array;
    result.messageArray.messages.messages_len = msg_count;
    result.messageArray.count = msg_count;
    
    printf("Found %d messages for '%s'.\n", msg_count, argp->client_name);
    return &result;
}

