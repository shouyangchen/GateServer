#ifndef REDISPOOL_H
#define REDISPOOL_H

#include <sw/redis++/redis++.h>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <vector>

class RedisPool {
public:
    RedisPool();
    ~RedisPool();
    
    std::shared_ptr<sw::redis::Redis> getConnection();
    void returnConnection(std::shared_ptr<sw::redis::Redis> conn);
    void close();

private:
    std::queue<std::shared_ptr<sw::redis::Redis>> available_connections_;
    std::mutex pool_mutex_;
    std::condition_variable pool_condition_;
    std::atomic<bool> closed_{false};
    
    std::string host_;
    int port_;
    std::string password_;
    size_t pool_size_;
    
    std::shared_ptr<sw::redis::Redis> createConnection();
};

#endif // REDISPOOL_H
