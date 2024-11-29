#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include "logger.h"
#include "common.h"

void signal_handle(int sig) {
    INFO() << "[Node] " << "Disconnected from gateway.";
    exit(0);
}

int main(int argc, char* argv[]) {
    Logger().setLevel(Logger::Level::DEBUG);
    if(signal(SIGPIPE, signal_handle) == SIG_ERR) {
        printf("error signal()");
        return errno;
    }
    if(argc < 2) {
        ERROR() << "Usage: ./node <node_name>\n\tnode_name: maximum 10 characters";
        exit(1);
    }

    message_t mesg;
    strcpy(mesg.node, argv[1]);


    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if(sock == -1) {
        ERROR() << "[Node] " << "Failed to open socket.";
        exit(1);
    }

    sockaddr_in server_add;

    server_add.sin_family = AF_INET;
    server_add.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_add.sin_port = htons(SERVER_PORT);

    int count = 0;
    while (count < 3) 
    {
        count++;
        if(connect(sock, (sockaddr*)&server_add, sizeof(server_add)) == -1) {
            ERROR() << "[Node] " << "Failed to connect gateway.";
        } else {
            count = 0;
            break;
        }
        sleep(3);
    }

    if(count == 3) {
        ERROR() << "[Node] " << "Failed to connect gateway over 3 times. Exit!";
        exit(1);
    }

    INFO() << "[Node] " << "Connected to gateway";

    count = 0;
    while(true || count < 3) {
        int value = rand() % 30 + 20;
        mesg.temperature = htons(value);
        int n = send(sock, &mesg, sizeof(message_t), 0);
        INFO() << "[Node] " << "Send " << value;
        if( n < 0) {
            ERROR() << "[Node] " << "Send failed.";
        }
        count++;
        sleep(1);
    }
    close(sock);

}