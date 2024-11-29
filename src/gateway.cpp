#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "logmgr.h"
#include "datamgr.h"
#include "connectionmgr.h"
#include "datamgr.h"
#include "storagemgr.h"

int main(int argc, char* argv[]) {
    g_stop = false;
    Logger().setLevel(Logger::Level::DEBUG);
    LogMgr logmgr;
    logmgr.exec();
    ConnectionMgr connectionmgr;
    connectionmgr.exec();
    ThreadPool tp(2);
    DataMgr datamgr;
    StorageMgr storagemgr;
    
    tp.enqueue([&]() {
        datamgr.exec();
    });
    tp.enqueue([&]() {
        storagemgr.exec();
    });

    DEBUG() << "[TEST] Auto close gateway after 20s";
    int count = 0;
    while (count < 20)
    {
        count++;
        sleep(1);
    }

    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_stop = true;
        g_cv.notify_all();
    }
    INFO() << "Terminate sensor gateway.";

    return 0;
}
