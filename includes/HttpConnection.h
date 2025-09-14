//
// Created by chenshouyang on 25-6-24.
//

#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include "const.h"

inline unsigned char ToHex(unsigned char x) // 将字符转化为十六进制
{
    return x > 9 ? x + 55 : x + 48; // 如果x大于9，则加上55，否则加上48（注意这是转化为大写字母表的16进制）
}

inline std::string UrlEncode(const std::string &str) // 对字符串进行URL编码
{
    std::string strTemp = ""; // 定义一个空字符串用于存储编码后的结果
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        // 判断是否仅有数字和字母构成
        if (isalnum((unsigned char)str[i]) || // 如果是数字或字母直接对字符进行拼结（isalnum是判断char存储的值是否为数值或者字符）
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ') // 为空字符
            strTemp += "+";
        else // 如果为中文或者其他字符
        {
            // 其他字符需要提前加%并且高四位和低四位分别转为16进制
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);   // 高四位转为16进制
            strTemp += ToHex((unsigned char)str[i] & 0x0F); // 低四位转为16进制
        }
    }
    return strTemp;
}

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicalSystem; // 允许LogicalSystem访问HttpConnection的私有成员
public:
    HttpConnection(tcp::socket socket);
    HttpConnection(boost::asio::io_context& ioc); // 构造函数，接受io_context对象
    void start();                // 启动连接处理
    void pre_parse_get_params(); // 解析GET请求的参数
	inline tcp::socket& get_socket() { return socket_; } // 获取套接字引用

private:
    void check_deadline();
    void write_response();
    void handlereq();
    tcp::socket socket_;
    beast::flat_buffer buffer_{8192};                                              // 缓冲区大小为8192字节
    http::request<http::dynamic_body> request_;                                    // 请求对象
    http::response<http::dynamic_body> response_;                                  // 响应对象
    net::steady_timer deadline_{socket_.get_executor(), std::chrono::seconds(60)}; // 定时器用于处理超时
    std::string get_url;
    std::unordered_map<std::string, std::string> get_params;
};



#endif // HTTPCONNECTION_H
