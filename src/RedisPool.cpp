#include "RedisPool.h"
#include "LogSystem.h"
#include "ConfiMgr.h"

std::shared_ptr<RedisPool> redispool; // 定义全局变量

RedisPool::RedisPool()
 {
    auto&config=ConfiMgr::getInstance();
    host_ = config["Redis"]["host"];
    port_ = std::stoi(config["Redis"]["port"]);
    password_ = config["Redis"]["passwd"];
    pool_size_ = std::stoul(config["Redis"]["pool_size"]);
    // 预创建连接池
    for (size_t i = 0; i < pool_size_; ++i) {
        auto conn = createConnection();
        if (conn) {
            available_connections_.push(conn);
        }
    }
    
    LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
        LogLevel::INFO, "Redis connection pool initialized with " + std::to_string(available_connections_.size()) + " connections"));
}

RedisPool::~RedisPool() {
    close();
}

std::shared_ptr<sw::redis::Redis> RedisPool::createConnection() {
    try {
        sw::redis::ConnectionOptions options;
        options.host = host_;
        options.port = port_;
        if (!password_.empty()) {
            options.password = password_;
        }
        
        auto conn = std::make_shared<sw::redis::Redis>(options);
        conn->ping(); // 测试连接
        return conn;
    } catch (const std::exception& e) {
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Failed to create Redis connection: " + std::string(e.what())));
        return nullptr;
    }
}

std::shared_ptr<sw::redis::Redis> RedisPool::getConnection() {
    std::unique_lock<std::mutex> lock(pool_mutex_);
    
    pool_condition_.wait(lock, [this] { 
        return !available_connections_.empty() || closed_.load(); 
    });
    
    if (closed_.load()) {
        return nullptr;
    }
    
    auto conn = available_connections_.front();
    available_connections_.pop();
    return conn;
}

void RedisPool::returnConnection(std::shared_ptr<sw::redis::Redis> conn) {
    if (!conn || closed_.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    available_connections_.push(conn);
    pool_condition_.notify_one();
}

void RedisPool::close() {
    closed_.store(true);
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    while (!available_connections_.empty()) {
        available_connections_.pop();
    }
    
    pool_condition_.notify_all();
}
