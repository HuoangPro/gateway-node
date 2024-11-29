#ifndef __COMMON__
#define __COMMON__

#include <mutex>
#include <condition_variable>

#define MAX_CONNECTION 9
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5555
#define BUFF_SIZE 1024
#define NODE_NAME_SIZE 10
#define SHM_NAME "/shm_sensordata"
#define SHM_SIZE 1024
#define INVALID -9999
#define STORAGE_NAME "storage"

std::mutex g_mutex;
std::condition_variable g_cv;
bool g_stop;

struct message_t
{
    char node[NODE_NAME_SIZE];
    int temperature;
};



#endif //__COMMON__