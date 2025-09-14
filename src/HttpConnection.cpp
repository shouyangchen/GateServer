//
// Created by chenshouyang on 25-6-24.
//

#include "HttpConnection.h"

#include <iostream>

#include "LogicalSystem.h"

HttpConnection::HttpConnection(tcp::socket socket)
    : socket_(std::move(socket))
{
    // 设置定时器的到期时间为60秒
}

HttpConnection::HttpConnection(boost::asio::io_context &ioc)
    : socket_(ioc)
{
}

void HttpConnection::start()
{
    // 启动连接处理
    auto self = shared_from_this(); // 获取 shared_ptr 的实例
    http::async_read(socket_, buffer_, request_,
                     [self](::boost::beast::error_code ec, std::size_t bytes_transferred)
                     {
                         try
                         {
                             if (ec)
                             {
                                 std::cout << "http read error" << ec.message() << "\n";
                                 return;
                             }
                             boost::ignore_unused(bytes_transferred); // 忽略传输的字节数
                             self->handlereq();
                             self->check_deadline();
                         }
                         catch (std::exception &e)
                         {
                             std::cout << "exception is" << e.what() << "\n";
                         }
                     });
}

void HttpConnection::handlereq()
{
    // 处理请求
    response_.version(request_.version()); // 设置响应版本
    response_.keep_alive(false);           // 设置连接不保持
    if (request_.method() == http::verb::get)
    {
        this->pre_parse_get_params();                                                         // 预解析GET请求的参数
        bool success = LogicalSystem::getInstance()->handle_get(get_url, shared_from_this()); // 处理GET请求
        if (!success)
        {
            this->response_.result(http::status::not_found); // 如果处理失败，返回404 Not Found
            this->response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "url not found\r\n";
            this->write_response();
            return;
        }
        this->response_.result(http::status::ok);
        this->response_.set(http::field::server, "GateServer");
        this->write_response();
    }
    if (request_.method() == http::verb::post)
    {
        bool success = LogicalSystem::getInstance()->handle_post(request_.target(), shared_from_this());
        if (!success)
        {
            this->response_.result(http::status::not_found);
            this->response_.set(http::field::content_type, "text/plain");
            beast::ostream(this->response_.body()) << "url not found\r\n";
            this->write_response();
            return;
        }
        this->response_.result(http::status::ok); // 如果处理失败，返回404 Not Found
        this->response_.set(http::field::server, "GateServer");
        this->write_response();
    }
}

void HttpConnection::write_response()
{ // 处理写入响应
    // 写入响应
    auto self = shared_from_this();                    // 获取 shared_ptr 的实例
    this->response_.content_length(response_.body().size()); // 设置内容长度
    http::async_write(this->socket_, this->response_,
                      [self](::boost::beast::error_code ec, std::size_t bytes_transferred) { // 处理写入响应
                          self->socket_.shutdown(tcp::socket::shutdown_send, ec);            // 关闭发送端
                          self->deadline_.cancel();                                          // 取消定时器
                      });
}

void HttpConnection::check_deadline()
{ // 检查定时器是否到期并处理超时
    // 检查定时器是否到期
    auto self = shared_from_this(); // 获取 shared_ptr 的实例
    deadline_.async_wait([self](::boost::beast::error_code ec)
                         {
                             if (ec)
                             {
                                 return; // 如果定时器被取消，直接返回
                             }
                             self->socket_.close(ec); // 关闭套接字
                         });
}

void HttpConnection::pre_parse_get_params()
{
    // 提取 URI
    auto uri = request_.target();
    // 查找查询字符串的开始位置（即 '?' 的位置）
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos)
    {                  // 如果没有查询参数
        get_url = uri; // 没有查询参数，直接使用 URI
        return;
    }

    get_url = uri.substr(0, query_pos);                   // 有的话就先保存uri
    std::string query_string = uri.substr(query_pos + 1); // 提取查询字符串部分
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos)
    {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos)
        {
            key = UrlEncode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码
            value = UrlEncode(pair.substr(eq_pos + 1));
            get_params[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // 处理最后一个参数对（如果没有 & 分隔符）
    if (!query_string.empty())
    {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos)
        {
            key = UrlEncode(query_string.substr(0, eq_pos));
            value = UrlEncode(query_string.substr(eq_pos + 1));
            get_params[key] = value;
        }
    }
}
