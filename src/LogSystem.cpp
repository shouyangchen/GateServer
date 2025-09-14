#include "LogSystem.h"
LogSystem::LogSystem()
{
    std::string current_path = boost::filesystem::current_path().string();
    this->init(current_path + "/logs"); // 初始化日志系统，设置日志目录为当前工作目录下的logs文件夹
}

LogQueue::LogQueue()
{
    // 初始化日志队列
}

void LogQueue::push(std::pair<LogLevel, std::string> log_message)
{
    std::unique_lock<std::mutex> lock(this->queue_mutex); // 确保线程安全
    log_queue.push(log_message);                          // 将日志消息添加到队列中
    queue_cond.notify_one();                              // 通知等待的线程有新日志可处理
}

std::pair<LogLevel, std::string> LogQueue::pop()
{
    auto logmessage = log_queue.front(); // 获取队列前端的日志消息
    log_queue.pop();                     // 从队列中移除该日志消息
    return logmessage;                   // 返回日志消息
}

bool LogQueue::empty() const
{
    std::unique_lock<std::mutex> lock(queue_mutex); // 确保线程安全
    return log_queue.empty(); // 检查队列是否为空
}

void LogQueue::run()
{
    std::shared_ptr<LogQueue> self = shared_from_this();  // 获取当前对象的shared_ptr
    this->running.store(true, std::memory_order_release); // 设置线程运行标志为true
    this->log_thread = std::thread([self]()
                                   {
        while(self->running.load(std::memory_order_acquire))
        {
            std::unique_lock<std::mutex> lock(self->queue_mutex); // 确保线程安全
            self->queue_cond.wait(lock,[self]{return !self->log_queue.empty() || !self->running.load(std::memory_order_acquire);}); // 等待有新日志可处理或线程停止
            
            while (!self->log_queue.empty() && self->running.load(std::memory_order_acquire)) {
                std::pair<LogLevel,std::string> log_message = self->pop(); // 获取日志消息
                lock.unlock(); // 解锁互斥锁
                LogSystem::getInstance()->log(log_message.first,log_message.second ); // 调用日志系统记录日志
                lock.lock(); // 重新加锁以继续循环
            }
        } });
}

void LogQueue::stop()
{
    this->running.store(false, std::memory_order_release); // 设置线程运行标志为false
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);   // 确保线程安全
        this->queue_cond.notify_all();                           // 通知所有等待的线程
    }
    if (this->log_thread.joinable())
    {
        this->log_thread.join(); // 等待日志处理线程结束
    }
    
    // 处理剩余的日志消息
    std::unique_lock<std::mutex> lock(this->queue_mutex);
    while (!this->log_queue.empty()) {
        auto log_message = this->pop(); // 清空剩余的日志消息
        lock.unlock();
        LogSystem::getInstance()->log(log_message.first, log_message.second);
        lock.lock();
    }
}