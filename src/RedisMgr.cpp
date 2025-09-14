#include "RedisMgr.h"
#include "LogSystem.h"

RedisMgr::RedisMgr() : connected_(false)
{
}

bool RedisMgr::connect(const std::string& host, int port, const std::string& password)
{
    host_ = host;
    port_ = port;
    password_ = password;
    
    try {
        // 优化连接选项
        sw::redis::ConnectionOptions options;
        options.host = host_;
        options.port = port_;
        if (!password_.empty()) {
            options.password = password_;
        }
        
        // 优化连接池设置
        options.socket_timeout = std::chrono::milliseconds(100);  // 减少超时时间
        options.connect_timeout = std::chrono::milliseconds(100);
        options.keep_alive = true;  // 保持连接活跃
        
        // 连接池设置
        sw::redis::ConnectionPoolOptions pool_opts;
        pool_opts.size = 3;  // 减少连接数
        pool_opts.wait_timeout = std::chrono::milliseconds(50);  // 减少等待时间
        pool_opts.connection_lifetime = std::chrono::minutes(10);
        
        // 创建较少的连接放入池中
        for (int i = 0; i < 3; ++i) {
            auto conn = std::make_shared<sw::redis::Redis>(options, pool_opts);
            conn->ping(); // 测试连接
            
            std::lock_guard<std::mutex> lock(pool_mutex_);
            connection_pool_.push(conn);
        }
        
        connected_ = true;
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::INFO, "Redis connected successfully to " + host + ":" + std::to_string(port)));
        
        return true;
    } catch (const std::exception& e) {
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis connection failed: " + std::string(e.what())));
        connected_ = false;
        return false;
    }
}

std::shared_ptr<sw::redis::Redis> RedisMgr::getConnection()
{
    std::unique_lock<std::mutex> lock(pool_mutex_);
    
    // 添加超时，避免无限等待
    if (!pool_condition_.wait_for(lock, std::chrono::milliseconds(100), 
                                  [this] { return !connection_pool_.empty(); })) {
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::WARN, "Redis connection pool timeout"));
        return nullptr;
    }
    
    auto conn = connection_pool_.front();
    connection_pool_.pop();
    return conn;
}

void RedisMgr::returnConnection(std::shared_ptr<sw::redis::Redis> conn)
{
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    connection_pool_.push(conn);
    pool_condition_.notify_one();
}

bool RedisMgr::set(const std::string& key, const std::string& value)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        conn->set(key, value);
        returnConnection(conn);
        return true;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis SET failed: " + std::string(e.what())));
        return false;
    }
}

std::string RedisMgr::get(const std::string& key)
{
    if (!connected_) return "";
    
    auto conn = getConnection();
    try {
        auto result = conn->get(key);
        returnConnection(conn);
        return result ? *result : "";
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis GET failed: " + std::string(e.what())));
        return "";
    }
}

bool RedisMgr::del(const std::string& key)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        bool result = conn->del(key) > 0;
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis DEL failed: " + std::string(e.what())));
        return false;
    }
}

bool RedisMgr::exists(const std::string& key)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        bool result = conn->exists(key) > 0;
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis EXISTS failed: " + std::string(e.what())));
        return false;
    }
}

// 列表操作
bool RedisMgr::lpush(const std::string& key, const std::string& value)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        conn->lpush(key, value);
        returnConnection(conn);
        return true;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis LPUSH failed: " + std::string(e.what())));
        return false;
    }
}

bool RedisMgr::rpush(const std::string& key, const std::string& value)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        conn->rpush(key, value);
        returnConnection(conn);
        return true;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis RPUSH failed: " + std::string(e.what())));
        return false;
    }
}

std::string RedisMgr::lpop(const std::string& key)
{
    if (!connected_) return "";
    
    auto conn = getConnection();
    try {
        auto result = conn->lpop(key);
        returnConnection(conn);
        return result ? *result : "";
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis LPOP failed: " + std::string(e.what())));
        return "";
    }
}

std::string RedisMgr::rpop(const std::string& key)
{
    if (!connected_) return "";
    
    auto conn = getConnection();
    try {
        auto result = conn->rpop(key);
        returnConnection(conn);
        return result ? *result : "";
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis RPOP failed: " + std::string(e.what())));
        return "";
    }
}

long long RedisMgr::llen(const std::string& key)
{
    if (!connected_) return 0;
    
    auto conn = getConnection();
    try {
        auto result = conn->llen(key);
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis LLEN failed: " + std::string(e.what())));
        return 0;
    }
}

std::vector<std::string> RedisMgr::lrange(const std::string& key, long long start, long long stop)
{
    if (!connected_) return {};
    
    auto conn = getConnection();
    try {
        std::vector<std::string> result;
        conn->lrange(key, start, stop, std::back_inserter(result));
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis LRANGE failed: " + std::string(e.what())));
        return {};
    }
}

// 集合操作
bool RedisMgr::sadd(const std::string& key, const std::string& member)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        conn->sadd(key, member);
        returnConnection(conn);
        return true;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis SADD failed: " + std::string(e.what())));
        return false;
    }
}

bool RedisMgr::srem(const std::string& key, const std::string& member)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        auto result = conn->srem(key, member);
        returnConnection(conn);
        return result > 0;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis SREM failed: " + std::string(e.what())));
        return false;
    }
}

bool RedisMgr::sismember(const std::string& key, const std::string& member)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        auto result = conn->sismember(key, member);
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis SISMEMBER failed: " + std::string(e.what())));
        return false;
    }
}

std::vector<std::string> RedisMgr::smembers(const std::string& key)
{
    if (!connected_) return {};
    
    auto conn = getConnection();
    try {
        std::vector<std::string> result;
        conn->smembers(key, std::back_inserter(result));
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis SMEMBERS failed: " + std::string(e.what())));
        return {};
    }
}

// 哈希操作
bool RedisMgr::hset(const std::string& key, const std::string& field, const std::string& value)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        conn->hset(key, field, value);
        returnConnection(conn);
        return true;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis HSET failed: " + std::string(e.what())));
        return false;
    }
}

std::string RedisMgr::hget(const std::string& key, const std::string& field)
{
    if (!connected_) return "";
    
    auto conn = getConnection();
    try {
        auto result = conn->hget(key, field);
        returnConnection(conn);
        return result ? *result : "";
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis HGET failed: " + std::string(e.what())));
        return "";
    }
}

bool RedisMgr::hdel(const std::string& key, const std::string& field)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        auto result = conn->hdel(key, field);
        returnConnection(conn);
        return result > 0;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis HDEL failed: " + std::string(e.what())));
        return false;
    }
}

bool RedisMgr::hexists(const std::string& key, const std::string& field)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        auto result = conn->hexists(key, field);
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis HEXISTS failed: " + std::string(e.what())));
        return false;
    }
}

// 过期时间
bool RedisMgr::expire(const std::string& key, long long seconds)
{
    if (!connected_) return false;
    
    auto conn = getConnection();
    try {
        auto result = conn->expire(key, std::chrono::seconds(seconds));
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis EXPIRE failed: " + std::string(e.what())));
        return false;
    }
}

long long RedisMgr::ttl(const std::string& key)
{
    if (!connected_) return -1;
    
    auto conn = getConnection();
    try {
        auto result = conn->ttl(key);
        returnConnection(conn);
        return result;
    } catch (const std::exception& e) {
        returnConnection(conn);
        LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
            LogLevel::ERROR, "Redis TTL failed: " + std::string(e.what())));
        return -1;
    }
}

void RedisMgr::close()
{
    std::lock_guard<std::mutex> lock(pool_mutex_);
    while (!connection_pool_.empty()) {
        connection_pool_.pop();
    }
    connected_ = false;
    LogQueue::getInstance()->push(std::make_pair<LogLevel,std::string>(
        LogLevel::INFO, "Redis connection closed"));
}