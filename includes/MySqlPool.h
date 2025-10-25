#ifndef MY_SQL_POOL_H
#define MY_SQL_POOL_H

#include "ConfiMgr.h"
#include "LogSystem.h"
#include <iostream>
class MySqlPool {
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& db, int poolSize)
        : url_(url), user_(user), pass_(pass), db_(db), poolSize_(poolSize) {
        for (int i = 0; i < poolSize_; ++i) {
            mysqlpp::Connection conn(this->db_.c_str(), this->url_.c_str(), this->user_.c_str(), this->pass_.c_str(), ConfiMgr::getInstance()["Mysql"]["port"].empty() ? 3306 : std::stoi(ConfiMgr::getInstance()["Mysql"]["port"]));
            if (conn.connected()) {
                available_connections_.push(std::move(conn));
            } else {
                LogQueue::getInstance()->push(std::make_pair(LogLevel::ERROR, "Failed to create MySQL connection"));
                std::cout << "Failed to create MySQL connection" << std::endl;
            }
        }
        
    }
    void close() {
        std::unique_lock<std::mutex> lock(pool_mutex_);
        this->closed_.store(true,std::memory_order_release);
        pool_condition_.notify_all(); // Notify all waiting threads to wake up
        while (!available_connections_.empty()) {
            available_connections_.pop();
        }
    }
    std::shared_ptr<mysqlpp::Connection> getConnection() {
        std::unique_lock<std::mutex> lock(pool_mutex_);
        pool_condition_.wait(lock, [this] { return !available_connections_.empty(); });
        
        auto conn = std::make_shared<mysqlpp::Connection>(std::move(available_connections_.front()));
        available_connections_.pop();
        return conn;
    }
    void releaseConnection(std::shared_ptr<mysqlpp::Connection> conn) {
        std::unique_lock<std::mutex> lock(pool_mutex_);
        available_connections_.push(std::move(*conn));
        pool_condition_.notify_one();
    }
    ~MySqlPool() {
        close();
    }
private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string db_;
    int poolSize_;
    std::atomic<bool> closed_{false};
    std::queue<mysqlpp::Connection> available_connections_;
    std::mutex pool_mutex_;
    std::condition_variable pool_condition_;
};

#endif // MY_SQL_POOL_H