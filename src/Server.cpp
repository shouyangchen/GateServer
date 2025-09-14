//
// Created by chenshouyang on 25-6-24.
//

#include "Server.h"
#include "singleton.h"
#include "AsioIOServicePool.h"
Server::Server(boost::asio::io_context &ioc, unsigned short port) :
std::enable_shared_from_this<Server>(), acceptor_(ioc,tcp::endpoint(tcp::v4(),port)) {

}


void Server::strat() {
    auto self = this->shared_from_this(); // 获取 shared_ptr 的实例
	auto& io_context =AsioIOServicePool::getInstance()->GetIOService(); // 获取io_context实例
	std::shared_ptr<HttpConnection>new_connection = std::make_shared<HttpConnection>(io_context); // 创建新的HttpConnection实例
    std::shared_ptr<boost::asio::strand<boost::asio::io_context::executor_type>> strand = 
        std::make_shared<boost::asio::strand<boost::asio::io_context::executor_type>>(io_context.get_executor());
    acceptor_.async_accept(new_connection->get_socket(),boost::asio::bind_executor(*strand,[self,new_connection](boost::beast::error_code ec) {
        try {
            //如果出错就放弃监听
            if (ec) {
                self->strat();
                return;
            }
			new_connection->start(); // 启动新的连接处理
            // 继续监听新的连接
            self->strat();
        }
        catch (std::exception &e) {

        }
    }));
}
