#ifndef LOGSYSTEM_H
#define LOGSYSTEM_H
#include "const.h"
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <mutex>
#include <queue>
#include <thread>
#include <memory>
#include <atomic>
#include <condition_variable>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class LogSystem : public singleton<LogSystem>
{
public:
    ~LogSystem() = default;

    void init(const std::string& log_dir) {//初始化日志记录
        boost::filesystem::path dir(log_dir);
        if (!boost::filesystem::exists(dir)) {// 检查目录是否存在
            boost::filesystem::create_directories(dir);// 如果不存在则创建目录
        }
        log_file_path = dir / "log.txt";
    }

    // 改进的日志方法，支持日志级别和自动时间戳
    void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex); // 线程安全
        std::ofstream log_file(log_file_path.string(), std::ios_base::app);
        if (log_file.is_open()) {
            std::string timestamp = boost::posix_time::to_simple_string(
                boost::posix_time::second_clock::local_time());
            std::string level_str = getLevelString(level);
            log_file << "[" << timestamp << "] [" << level_str << "] " << message << std::endl;
            log_file.close();
        }
    }

    // 兼容旧版本的日志方法
    void log(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    // 便捷方法
    void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
    void info(const std::string& message) { log(LogLevel::INFO, message); }
    void warn(const std::string& message) { log(LogLevel::WARN, message); }
    void error(const std::string& message) { log(LogLevel::ERROR, message); }

private:
    boost::filesystem::path log_file_path; // 日志文件路径
    std::mutex log_mutex; // 互斥锁用于线程安全
    
    std::string getLevelString(LogLevel level) {
        switch(level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    LogSystem(const LogSystem&) = delete; // 禁止拷贝构造
    LogSystem& operator=(const LogSystem&) = delete; // 禁止赋值操作
    LogSystem(LogSystem&&) = delete; // 禁止移动构造
    LogSystem& operator=(LogSystem&&) = delete; // 禁止移动赋值
    friend class singleton<LogSystem>; // 允许singleton访问私有成员
    LogSystem(); // 私有构造函数确保只能通过singleton获取实例
};


class LogQueue : public singleton<LogQueue>, public std::enable_shared_from_this<LogQueue> {
    friend class singleton<LogQueue>; // 允许singleton访问私有成员
    LogQueue(const LogQueue&) = delete; // 禁止拷贝构造
    LogQueue& operator=(const LogQueue&) = delete; // 禁止赋值操作
    LogQueue(LogQueue&&) = delete; // 禁止移动构造
    LogQueue& operator=(LogQueue&&) = delete; // 禁止移动赋值
private:
    std::queue<std::pair<LogLevel,std::string>>log_queue; // 日志队列
    mutable std::mutex queue_mutex; // 互斥锁用于线程安全
    std::condition_variable queue_cond; // 条件变量用于通知
    LogQueue();
    std::thread log_thread; // 日志处理线程
    std::atomic<bool> running{false}; // 线程运行标志，初始为false
public:
    void push(std::pair<LogLevel,std::string>);
    std::pair<LogLevel,std::string> pop();
    bool empty()const;
    void run();
    void stop();
};

#endif