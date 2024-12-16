/* messaging.x - RPC specification for messaging server */

struct Message {
    string sender<100>;
    string recipient<100>;
    string text<256>;
    string timestamp<32>;
};

struct MessageArray {
    Message messages<>;
    int count;
};

struct RegisterRequest {
    string client_name<>;
};

struct SendMessageRequest {
    string sender<>;
    string recipient<>;
    string text<>;
};

struct FetchMessageRequest {
    string client_name<>;
};

struct FetchMessageResponse {
    MessageArray messageArray;
};

program MESSAGING_PROG {
    version MESSAGING_VERS {
        int register_client(RegisterRequest) = 1;
        int send_message(SendMessageRequest) = 2;
        FetchMessageResponse fetch_message(FetchMessageRequest) = 3;
    } = 1;
} = 0x20000001;
