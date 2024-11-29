#ifndef __CONNECTIONMGR__
#define __CONNECTIONMGR__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <sys/shm.h>
#include "common.h"
#include "logger.h"
#include "threadpool.h"

class ConnectionMgr
{
private:
    int mserver_port = SERVER_PORT;
    int mlisten_sock = -1;
    sockaddr_in mserver_addr;
    ThreadPool mthreadpool{5};
    std::thread mlisten_thread;
    int mshm_fd;
    void* mshm_ptr;

private:
    void listenToClient(int sock_fd);
    void acceptToClient();

public:
    ConnectionMgr();
    ~ConnectionMgr();
    void exec();
};

ConnectionMgr::ConnectionMgr() {
    mlisten_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(mlisten_sock == -1) {
        ERROR() << "[ConnectionMgr] " << "Failed to open socket with port " << mserver_port;
        exit(1);
    }

    mserver_addr.sin_family = AF_INET;
    mserver_addr.sin_addr.s_addr = INADDR_ANY;
    mserver_addr.sin_port = htons(mserver_port);

    if(bind(mlisten_sock, (sockaddr*)&mserver_addr, sizeof(mserver_addr)) == -1) {
        ERROR() << "[ConnectionMgr] " << "Failed to bind socket.";
        exit(1);
    }

    if (listen(mlisten_sock, MAX_CONNECTION) == -1) {
        ERROR() << "[ConnectionMgr] " << "Failed to listen socket.";
        exit(1);
    }

    INFO() << "[ConnectionMgr] " << "Listen with port " << mserver_port;

    mshm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if(mshm_fd == -1) {
        perror("shm_open");
        ERROR() << "[ConnectionMgr] " << "Failed to open shared memory '" << SHM_NAME << "'";
        exit(1);
    }
    DEBUG() << "[ConnectionMgr] " << "Shared memory open";

    if(ftruncate(mshm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        ERROR() << "[ConnectionMgr] " << "Failed to truncate shared memory '" << SHM_NAME << "'";
        exit(1);
    }

    mshm_ptr = mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mshm_fd, 0);
    if (mshm_ptr == MAP_FAILED) {
        perror("mmap");
        ERROR() << "[ConnectionMgr] " << "Failed to map shared memory";
        exit(1);
    }

    DEBUG() << "[ConnectionMgr] " << "Shared memory is mapped";

}

ConnectionMgr::~ConnectionMgr() {
    close(mlisten_sock);
    DEBUG() << "[ConnectionMgr] " << "Destructor. Socket is closed";

    if(munmap(mshm_ptr, SHM_SIZE) == -1) {
        perror("munmap");
        ERROR() << "[ConnectionMgr] " << "Failed to unsmap shared memory";
        exit(1);
    }

    if (close(mshm_fd) == -1) {
        perror("close");
        ERROR() << "[ConnectionMgr] " << "Failed to close shared memory";
        exit(1);
    }

    if (shm_unlink(SHM_NAME) == -1) {
        perror("close");
        ERROR() << "[ConnectionMgr] " << "Failed to close shared memory";
        exit(1);
    }
}

void ConnectionMgr::exec() {
    std::thread accept_thread([&](){
        acceptToClient();
    });
    accept_thread.detach();
}

void ConnectionMgr::listenToClient(int sock_fd) {
    char buff[BUFF_SIZE];
    message_t mesg;
    while (true)
    {
        bool requestClose = false;
        int n = recv(sock_fd, buff, sizeof(buff), 0);
        if(n <= 0) {
            INFO() << "[ConnectionMgr] " << "Connection is disconnected.";
            close(sock_fd);
            requestClose = true;
        }

        buff[n] = '\0';
        message_t mesg;
        if(requestClose) {
            mesg.temperature = INVALID;
        }
        else {
            mesg = *((message_t* )buff);
            mesg.temperature = ntohs(mesg.temperature);
            INFO() << "[ConnectionMgr] " << "Receive from node '" << mesg.node << "': " << mesg.temperature;
        }
        // 

        {
            std::unique_lock<std::mutex> lock(g_mutex);
            if(g_stop) break;
            std::memcpy(mshm_ptr, (void*)&mesg, sizeof(mesg));
        }
        DEBUG() << "[ConnectionMgr] " << "Write to shm node '" << mesg.node << "': " << mesg.temperature;
        g_cv.notify_all();
        if(requestClose) break;
    }
    

}

void ConnectionMgr::acceptToClient() {
    while (true)
    {
        int client_sock;
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        client_sock = accept(mlisten_sock, (sockaddr*)&client_addr, &addr_len);

        if (client_sock < 0) {
            ERROR() << "[ConnectionMgr] " << "Failed to accept connection.";
            continue;
        }

        INFO() << "[ConnectionMgr] " << "Connection is established."; 

        mthreadpool.enqueue([&](){
            listenToClient(client_sock);
        });

    }
}

#endif //__CONNECTIONMGR__

