#ifndef REDISMGR_H
#define REDISMGR_H

#include "const.h"
#include <sw/redis++/redis++.h>
#include <string>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

class RedisMgr : public singleton<RedisMgr>
{
    friend class singleton<RedisMgr>;
public:
    ~RedisMgr() = default;
    
    bool connect(const std::string& host = "127.0.0.1", int port = 6380, const std::string& password = "");
    
    // 基本操作
    bool set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    
    // 列表操作
    bool lpush(const std::string& key, const std::string& value);
    bool rpush(const std::string& key, const std::string& value);
    std::string lpop(const std::string& key);
    std::string rpop(const std::string& key);
    long long llen(const std::string& key);
    std::vector<std::string> lrange(const std::string& key, long long start, long long stop);
    
    // 集合操作
    bool sadd(const std::string& key, const std::string& member);
    bool srem(const std::string& key, const std::string& member);
    bool sismember(const std::string& key, const std::string& member);
    std::vector<std::string> smembers(const std::string& key);
    
    // 有序集合操作
    bool zadd(const std::string& key, double score, const std::string& member);
    bool zrem(const std::string& key, const std::string& member);
    long long zcard(const std::string& key);
    std::vector<std::string> zrange(const std::string& key, long long start, long long stop);
    
    // 哈希操作
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    std::string hget(const std::string& key, const std::string& field);
    bool hdel(const std::string& key, const std::string& field);
    bool hexists(const std::string& key, const std::string& field);
    
    // 过期时间
    bool expire(const std::string& key, long long seconds);
    long long ttl(const std::string& key);
    
    void close();

private:
    RedisMgr();
    RedisMgr(const RedisMgr&) = delete;
    RedisMgr& operator=(const RedisMgr&) = delete;
    
    // 连接池
    std::queue<std::shared_ptr<sw::redis::Redis>> connection_pool_;
    std::mutex pool_mutex_;
    std::condition_variable pool_condition_;
    
    std::shared_ptr<sw::redis::Redis> getConnection();
    void returnConnection(std::shared_ptr<sw::redis::Redis> conn);
    
    std::string host_;
    int port_;
    std::string password_;
    bool connected_;
};

#endif // REDISMGR_H