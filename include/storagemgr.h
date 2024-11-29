#ifndef __STORAGEMGR__
#define __STORAGEMGR__
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>
#include <map>
#include <cstring>
#include "common.h"
#include "logger.h"

using node_data = message_t;

std::string toString(const node_data& data) {
    return std::string(data.node) + ":" + std::to_string(data.temperature);
}

class StorageMgr {
private:
    int mshm_fd;
    void* mshm_ptr;
    int mstorage_fd;

private:
    void writeToStorage(const node_data& data);

public:
    StorageMgr();
    ~StorageMgr();
    void exec();

};

StorageMgr::StorageMgr() {

    mstorage_fd = open(STORAGE_NAME, O_RDWR | O_CREAT | O_APPEND, 0644);

    if(mstorage_fd == -1) {
        perror("open");
        ERROR() << "[StorageMgr] " << "Failed to open storage file '" << STORAGE_NAME << "'";
        exit(1);
    }
    DEBUG() << "[StorageMgr] " << "Open storage";


    mshm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if(mshm_fd == -1) {
        perror("shm_open");
        ERROR() << "[StorageMgr] " << "Failed to open shared memory '" << SHM_NAME << "'";
        exit(1);
    }

    if(ftruncate(mshm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        ERROR() << "[StorageMgr] " << "Failed to truncate shared memory '" << SHM_NAME << "'";
        exit(1);
    }

    mshm_ptr = mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mshm_fd, 0);
    if (mshm_ptr == MAP_FAILED) {
        perror("mmap");
        ERROR() << "[StorageMgr] " << "Failed to map shared memory";
        exit(1);
    }

}

StorageMgr::~StorageMgr() {
    DEBUG() << "[StorageMgr] " << "Destructor";

    if(munmap(mshm_ptr, SHM_SIZE) == -1) {
        perror("munmap");
        ERROR() << "[StorageMgr] " << "Failed to unsmap shared memory";
        exit(1);
    }

    if (close(mshm_fd) == -1) {
        perror("close");
        ERROR() << "[StorageMgr] " << "Failed to close shared memory";
        exit(1);
    }

    close(mstorage_fd);
    DEBUG() << "[StorageMgr] " << "Close storage";
}

void StorageMgr::exec() {
    DEBUG() << "[StorageMgr] " << "Fetching data...";
    while (true)
    {
        node_data d;
        {
            std::unique_lock<std::mutex> lock(g_mutex);
            g_cv.wait(lock);
            if(g_stop) break;
            d = *((node_data*)mshm_ptr);
        }
        DEBUG() << "[StorageMgr] " << "Read from shm '" << d.node << "': " << d.temperature;
        writeToStorage(d);
    }
    
}

void StorageMgr::writeToStorage(const node_data& data) {
    auto s = toString(data) + "\n";
    write(mstorage_fd, s.c_str(), s.length());
}


#endif //__STORAGEMGR__