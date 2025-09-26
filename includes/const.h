//
// Created by chenshouyang on 25-6-24.
//

#ifndef CONST_H
#define CONST_H
#include <memory>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <memory>
#include "singleton.h"
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include "RedisPool.h"
#include <mysql++/mysql++.h>
#include <mysql++/connection.h>
#include <mysql++/query.h>
extern std::shared_ptr<RedisPool> redispool; // Redis连接池实例

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>


using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
enum ErrorCodes {
    Success = 0,
    Error_Josn = 1001,// JSON解析错误
    RPCFail = 1002,// RPC调用失败
    VarifyEXpired = 1003,// 验证码过期
    VarifyError = 1004,// 验证码错误
    UerExist = 1005,// 用户已存在
    MySqlError = 1006,// MySQL操作错误
    RedisError = 1007,// Redis操作错误
    EmailIsRegister=1008,//邮箱已注册
    UserNotExist=1009,//用户不存在
    UserPasswdError=1010,//用户密码错误
    EmailIsNotMatch=1011,//邮箱不匹配
};

struct user_info{
    std::uint64_t uid;
    std::string email;
    std::string passwd;
};

#endif //CONST_H
