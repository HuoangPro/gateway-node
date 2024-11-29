#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cerrno>
#include <thread>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <atomic>

#define DEBUG() Logger().debug()
#define WARN() Logger().warn()
#define INFO() Logger().info()
#define ERROR() Logger().error()
#define FATAL() Logger().fatal()
class Logger
{
public:
    enum class Level {
        DEBUG,
        WARNING,
        INFO,
        ERROR,
        FATAL,

        MAX
    };
private:
    inline static Level s_mlevel = Level::INFO;
    inline static std::atomic<int> s_midx{0};
    inline static std::mutex s_mmutex;
    inline static int s_mfd = -1;
    Level mlevel = Level::INFO;
    std::stringstream mss;
    bool mauto_new_line = true;
public:
    Logger() {}
    ~Logger();
    template <class T>
    Logger& operator <<(T data) { mss << data; return *this; }
    Logger& setLevel(Level value) { s_mlevel = value; return *this;}
    Logger& setFD(int fd) { s_mfd = fd; return *this;}
    Logger& autoNewLine(bool value) { mauto_new_line = value; return *this;}
    Logger& resetIdx() { s_midx = 0; return *this;}
    Logger& debug();
    Logger& warn();
    Logger& info();
    Logger& error();
    Logger& fatal();
    Logger& flush();
};
#define MESG(color, type) mss << color << ++s_midx << ": " << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << ": " type "\033[0m: " << gettid() << ": ";
Logger& Logger::debug() {
    mlevel = Level::DEBUG;
    if(mlevel < s_mlevel) return *this;
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&currentTime);
    
    MESG("\033[0;35m","DEBUG")
    return *this;
}

Logger& Logger::warn() {
    mlevel = Level::WARNING;
    if(mlevel < s_mlevel) return *this;
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&currentTime);
    
    MESG("\033[0;33m","WARN ")
    return *this;
}

Logger& Logger::info() {
    mlevel = Level::INFO;
    if(mlevel < s_mlevel) return *this;
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&currentTime);
    
    MESG("\033[0;34m","INFO ")
    return *this;
}

Logger& Logger::error() {
    mlevel = Level::ERROR;
    if(mlevel < s_mlevel) return *this;
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&currentTime);
    
    MESG("\033[0;31m","ERROR")
    return *this;
}

Logger& Logger::fatal() {
    mlevel = Level::FATAL;
    if(mlevel < s_mlevel) return *this;
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&currentTime);
    
    MESG("\033[0;31m","FATAL")
    return *this;
}
#undef MESG

Logger& Logger::flush() {
    if(mlevel < s_mlevel) return *this;
    if(mss.str().length() == 0) return *this;
    if(mauto_new_line) mss << "\n";
    std::lock_guard<std::mutex> l(s_mmutex);
    if(s_mfd == -1)
        std::cout << mss.str();
    else
        write(s_mfd, mss.str().c_str(), mss.str().length());

    mss.clear();
    if(mlevel == Level::FATAL) exit(1);

    return *this;
}

Logger::~Logger() {
    flush();
}

#endif // __LOGGER_H__