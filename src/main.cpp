#include <iostream>
#include "const.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include "ConfiMgr.h"
#include "Server.h"
#include <hiredis/hiredis.h>
#include "LogSystem.h"


int main(int argc, char *argv[])
{

        try {
        mysqlpp::Connection conn("onlinetalk", "127.0.0.1", "chenshouyang", "123456", 3307);
   } catch (std::exception &e) {
        std::cerr << "MySQL connection error: " << e.what() << std::endl;
   
   }  
    try
    {

        auto g_log_system = LogSystem::getInstance();
        g_log_system->init("logs");                   // 初始化日志系统，设置日志目录为当前工作目录下的logs文件夹
        g_log_system->info("GateServer starting..."); // 记录服务器启动日志
        auto g_log_queue = LogQueue::getInstance();
        g_log_queue->run(); // 启动日志队列处理线程
        auto g_config_mgr = ConfiMgr::getInstance();
        redispool = std::make_shared<RedisPool>(); // 创建Redis连接池实例
        
        std::string port_str = g_config_mgr["GateServer"]["port"]; // 从配置中获取端口号
        int port = std::atoi(port_str.c_str());                    // 将端口号转换为整数
        std::cout << "GateServer port is " << port << std::endl;

        net::io_context ioc{4}; // 创建io_context对象
        boost::asio::signal_set signals_asio(ioc, SIGINT, SIGTERM);
        signals_asio.async_wait([&ioc](const boost::system::error_code &err, int signal_nums)
                           {
                               if (err)
                               {
                                   return;
                               }
                               ioc.stop(); // 停止io_context
                           });
        std::make_shared<Server>(ioc, port)->strat(); // 创建服务器实例并开始监听
        std::cout << "Server is running on port " << port << std::endl;
        ioc.run(); // 运行io_context，开始处理异步操作
    }

    catch (const std::exception &e)
    {
        std::cerr << "error is" << e.what() << std::endl;
        LogQueue::getInstance()->push(std::make_pair(LogLevel::ERROR, "Exception occurred: " + std::string(e.what())));
        LogQueue::getInstance()->stop(); // 停止日志队列处理线程
        return -1;
    }


}

