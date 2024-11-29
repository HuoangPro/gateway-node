#ifndef __DATAMGR__
#define __DATAMGR__
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>
#include <map>
#include "common.h"
#include "logger.h"

using node_data = message_t;

class DataMgr {
private:
    std::vector<node_data> mnodes;
    int mshm_fd;
    void* mshm_ptr;
    std::map<std::string, int> mnodeMap;

private:
    void predictTemperature();

public:
    DataMgr();
    ~DataMgr();
    void exec();

};

DataMgr::DataMgr() {
    mshm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if(mshm_fd == -1) {
        perror("shm_open");
        ERROR() << "[DataMgr] " << "Failed to open shared memory '" << SHM_NAME << "'";
        exit(1);
    }

    if(ftruncate(mshm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        ERROR() << "[DataMgr] " << "Failed to truncate shared memory '" << SHM_NAME << "'";
        exit(1);
    }

    mshm_ptr = mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mshm_fd, 0);
    if (mshm_ptr == MAP_FAILED) {
        perror("mmap");
        ERROR() << "[DataMgr] " << "Failed to map shared memory";
        exit(1);
    }

}

DataMgr::~DataMgr() {
    DEBUG() << "[DataMgr] " << "Destructor";

    if(munmap(mshm_ptr, SHM_SIZE) == -1) {
        perror("munmap");
        ERROR() << "[DataMgr] " << "Failed to unsmap shared memory";
        exit(1);
    }

    if (close(mshm_fd) == -1) {
        perror("close");
        ERROR() << "[DataMgr] " << "Failed to close shared memory";
        exit(1);
    }

}

void DataMgr::exec() {
    DEBUG() << "[DataMgr] " << "Fetching data...";
    while (true)
    {
        node_data d;
        {
            std::unique_lock<std::mutex> lock(g_mutex);
            g_cv.wait(lock);
            if(g_stop) break;
            d = *((node_data*)mshm_ptr);
        }
        DEBUG() << "[DataMgr] " << "Read from shm '" << d.node << "': " << d.temperature;
        mnodeMap[d.node] = d.temperature;
        if(d.temperature == INVALID) mnodeMap.erase(d.node);
        DEBUG() << "[DataMgr] " << "Remove '" << d.node << "'";
        predictTemperature();
    }
    
}

void DataMgr::predictTemperature() {
    int sum = 0;
    for(const auto& node: mnodeMap) sum += node.second;
    float avg = sum*1.0/mnodeMap.size();
    if(avg <= 30) INFO() << "[GATEWAY] " << "Cold";
    else if(avg <= 40) INFO() << "[GATEWAY] " << "Normal";
    else INFO() << "[GATEWAY] " << "Hot";
}


#endif //__DATAMGR__