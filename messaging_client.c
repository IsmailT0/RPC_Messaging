#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "messaging.h"

void print_menu()
{
    printf("\n1. Register\n2. Send Message\n3. Fetch Messages\n4. Exit\n");
    printf("Choose an option: ");
}

void messaging_prog_1(char *host)
{
    CLIENT *clnt;

    // RPC client setup
    clnt = clnt_create(host, MESSAGING_PROG, MESSAGING_VERS, "udp");
    if (clnt == NULL)
    {
        clnt_pcreateerror(host);
        exit(1);
    }

    char client_name[100]; // To store the client's name
    int registered = 0;    // To track registration status

    while (1)
    {
        print_menu();
        int choice;
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
        { // Register
            if (registered)
            {
                printf("You are already registered as '%s'.\n", client_name);
                break;
            }

            printf("Enter your name: ");
            scanf("%s", client_name);

            RegisterRequest req = {client_name};
            int *result = register_client_1(&req, clnt);
            if (result == NULL)
            {
                clnt_perror(clnt, "RPC call failed");
            }
            else if (*result == 1)
            {
                printf("Successfully registered as '%s'.\n", client_name);
                registered = 1;
            }
            else
            {
                printf("Registration failed. Name might already be taken.\n");
            }
            break;
        }

        case 2:
        { // Send Message
            if (!registered)
            {
                printf("You must register first.\n");
                break;
            }

            char recipient[100];
            char text[256];

            printf("Enter recipient's name: ");
            scanf("%s", recipient);
            printf("Enter message: ");
            getchar(); // Clear buffer
            fgets(text, 256, stdin);
            text[strcspn(text, "\n")] = '\0'; // Remove newline

            SendMessageRequest req = {client_name, recipient, text};
            int *result = send_message_1(&req, clnt);
            if (result == NULL)
            {
                clnt_perror(clnt, "RPC call failed");
            }
            else if (*result == 1)
            {
                printf("Message sent to '%s'.\n", recipient);
            }
            else
            {
                printf("Failed to send message. Recipient not found.\n");
            }
            break;
        }

        case 3:
        { // Fetch Messages
            if (!registered)
            {
                printf("You must register first.\n");
                break;
            }

            FetchMessageRequest req = {client_name};
            FetchMessageResponse *response = fetch_message_1(&req, clnt);
            if (response == NULL)
            {
                clnt_perror(clnt, "RPC call failed");
            }
            else
            {
                int count = response->messageArray.count;
                if (count > 0)
                {
                    printf("\nYou have %d message(s):\n", count);
                    for (int i = 0; i < count; i++)
                    {
                        Message *msg = &response->messageArray.messages.messages_val[i];
                        printf("\n%d. From: %s\nTime: %s\nMessage: %s\n",
                               i + 1,
                               msg->sender,
                               msg->timestamp,
                               msg->text);
                    }
                }
                else
                {
                    printf("No new messages.\n");
                }
            }
            break;
        }

        case 4:
        { // Exit
            if (registered) {
                RegisterRequest req = {client_name};
                int *result = deregister_client_1(&req, clnt);
                if (result != NULL && *result == 1) {
                    printf("Successfully deregistered.\n");
                }
            }
            printf("Exiting. Goodbye!\n");
            clnt_destroy(clnt);
            exit(0);
        }

        default:
            printf("Invalid option. Please try again.\n");
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s server_host\n", argv[0]);
        exit(1);
    }
    messaging_prog_1(argv[1]);
    return 0;
}
