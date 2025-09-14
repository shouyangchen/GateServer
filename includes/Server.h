//
// Created by chenshouyang on 25-6-24.
//

#ifndef SERVER_H
#define SERVER_H
#include "const.h"
#include "HttpConnection.h"

class Server :public std::enable_shared_from_this<Server>{// 通过enable_shared_from_this可以在类内部获取shared_ptr实例
public:
    Server(boost::asio::io_context &ioc,unsigned short port);
    void strat();
private:
    tcp::acceptor acceptor_;// 接受连接的接受器
    net::io_context ioc_;// io_context对象用于异步操作
};


#endif //SERVER_H
