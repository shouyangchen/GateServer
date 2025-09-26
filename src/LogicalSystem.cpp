//
// Created by chenshouyang on 25-6-24.
//
#include "LogicalSystem.h"
#include "ConfiMgr.h"
#include "HttpConnection.h"
#include "MySqlMgr.h"
#include "RedisPool.h"
#include "VarfiyGrpcClient.h"
#include "const.h"
#include "gRPCMrg.h"
#include "message.pb.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/websocket/detail/hybi13.hpp>
#include <cstdint>
#include <cstdlib>
#include <grpcpp/client_context.h>
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <memory>
#include <ostream>
#include <string>


void LogicalSystem::reg_get(std::string url, HttpHandler handler) {
  // 注册处理GET请求的处理器
  get_handlers.insert(make_pair(url, handler));
}

LogicalSystem::LogicalSystem() {
  reg_get("/get_test", [](const std::shared_ptr<HttpConnection> conn) {
    // 处理GET请求的逻辑
    LogSystem::getInstance()->info(
        "Received GET request on /get_test endpoint");
    beast::ostream(conn->response_.body())
        << "receive get request\r\n"; // 响应内容
    int i = 0;
    for (auto &elem : conn->get_params) {
      i++;
      beast::ostream(conn->response_.body())
          << " params" << i << " key:" << elem.first << " value:" << elem.second
          << "\r\n"; // 输出参数
      LogSystem::getInstance()->debug("GET parameter " + std::to_string(i) +
                                      ": " + elem.first + " = " + elem.second);
    }
  });
  reg_post("/get_varify_code", [](std::shared_ptr<HttpConnection> connection) {
    auto body_str = boost::beast::buffers_to_string(
        connection->request_.body().data()); // 获取请求体内容
    std::cout << "Received POST request with body: " << body_str << std::endl;
    connection->response_.set(http::field::content_type,
                              "text/json"); // 设置响应的Content-Type为JSON
    // 处理POST请求的逻辑
    Json::Value root;
    Json::Reader reader;
    Json::Value src_root;
    bool pares_success = reader.parse(body_str, src_root); // 解析JSON数据
    if (!pares_success) {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR, "Failed to parse JSON request body: " + body_str));
      root["error"] = ErrorCodes::Error_Josn; // 设置错误码
      std::string error_message = root.toStyledString();
      beast::ostream(connection->response_.body())
          << error_message; // 输出错误信息
      return true;
    }
    if (!src_root.isMember("email")) {
      std::cout << "Email field is missing in request body." << std::endl;
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::WARN, "Email field is missing in request body"));
      root["error"] = ErrorCodes::Error_Josn; // 设置错误码
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      return true;
    }
    auto email = src_root["email"].asString();
    LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
        LogLevel::INFO,
        "Processing verification code request for email: " + email));
    GetVarifyRsp rsp = VarfiyGrpcClient::getInstance()->GetVarifyCode(
        email); // 调用gRPC客户端获取验证码
    std::cout << "Received email: " << email << std::endl;

    if (rsp.error() == ErrorCodes::Success) {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::INFO,
          std::string("Verification code sent successfully to email: " +
                      email)));
    } else {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR,
          std::string(
              std::string("Failed to send verification code to email: ") +
              email + std::string(", error code: ") +
              std::to_string(rsp.error()))));
    }

    root["error"] = rsp.error(); // 设置成功码
    root["email"] = src_root["email"];
    std::string jsonstr = root.toStyledString();
    beast::ostream(connection->response_.body()) << jsonstr; // 输出JSON响应
    return true;
  });

  reg_post("/user_register", [](std::shared_ptr<HttpConnection> connection) {
    auto body_str = boost::beast::buffers_to_string(
        connection->request_.body().data()); // 获取请求体内容
    std::cout << "Received POST request with body: " << body_str << std::endl;
    connection->response_.set(http::field::content_type,
                              "text/json"); // 设置响应的Content-Type为JSON
    Json::Value root;
    Json::Reader reader;
    Json::Value src_root;
    bool parse_success = reader.parse(body_str, src_root); // 解析JSON数据
    if (!parse_success) {
      std::cout << "Failed to parse JSON request body: " << body_str
                << std::endl;
      root["error"] = ErrorCodes::Error_Josn; // 设置错误码
      std::string error_message = root.toStyledString();
      beast::ostream(connection->response_.body())
          << error_message; // 输出错误信息
      return true;
    }

    auto email = src_root["email"].asString();
    auto name = src_root["user"].asString();
    auto pwd = src_root["passwd"].asString();
    auto confirm = src_root["confirm"].asString();

    std::string varify_code;
    auto redis_conn = redispool->getConnection();
    auto b_get_varfiy_code = redis_conn->get(
        "code_" + src_root["email"].asString()); // 从Redis获取验证码
    if (b_get_varfiy_code->empty()) {
      std::cout << "The varify code is not exist in redis or expired!"
                << std::endl;
      root["error"] = ErrorCodes::VarifyEXpired; // 设置错误码
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      return true;
    }
    if (b_get_varfiy_code->compare(src_root["varify_code"].asString()) != 0) {
      std::cout << "The varify code is not match!" << std::endl;
      root["error"] = ErrorCodes::VarifyError; // 设置错误码
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      return true;
    }
    bool b_usr_exit = redis_conn->exists(src_root["email"].asString() + "_usr");
    if (b_usr_exit) {
      std::cout << "The user is already exist!" << std::endl;
      root["error"] = ErrorCodes::UerExist; // 设置错误码
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      return true;
    }
    // 如果没有错误，则继续处理注册逻辑
    // 在mysql中查找用户逻辑
    auto result = MySqlMgr::getInstance()->RegUser(name, email,
                                                   pwd); // 调用MySqlMgr注册用户
    if (result == 0 || result == -1) {
      std::cout << "Failed to register user: " << name
                << ", error code: " << result << std::endl;
      root["error"] = ErrorCodes::MySqlError; // 设置错误码
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      redispool->returnConnection(redis_conn);
      return true;
    }
    if (result == -2) {
      std::cout << "Failed to register user: " << name
                << ", error code: " << result << std::endl;
      root["error"] = ErrorCodes::EmailIsRegister; // 设置错误码
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      redispool->returnConnection(redis_conn);
    }
    std::cout << "User registered successfully: " << name << std::endl;
    ///////
    root["error"] = ErrorCodes::Success; // 设置成功码
    root["email"] = src_root["email"];
    root["user"] = src_root["user"];
    root["passwd"] = src_root["passwd"];
    root["confirm"] = src_root["confirm"];
    root["varify_code"] = src_root["varify_code"];
    root["uid"] = result;
    std::cout << result << std::endl;
    std::string jsonstr = root.toStyledString();
    beast::ostream(connection->response_.body()) << jsonstr; // 输出JSON响应
    redispool->returnConnection(redis_conn);                 // 归还Redis连接
    return true;
  });

  reg_post("/user_login", [](std::shared_ptr<HttpConnection> connection) {
    auto msessage_body = beast::buffers_to_string(
        connection->request_.body().data()); // 获取请求体内容
    connection->response_.set(http::field::content_type,
                              "text/json"); // 设置响应的Content-Type为JSON
    Json::Value root;
    Json::Reader reader;
    Json::Value src_root;
    bool parse_success = reader.parse(msessage_body, src_root); // 解析JSON数据
    if (!parse_success) {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR,
          "Failed to parse JSON request body: " + msessage_body));
      std::cout << "Failed to parse JSON request body: " << msessage_body
                << std::endl;
      root["error"] = ErrorCodes::Error_Josn; // 设置错误码
      auto error_message = root.toStyledString();
      beast::ostream(connection->response_.body())
          << error_message; // 输出错误信息
      return true;
    }
    root["error"] = ErrorCodes::Success; // 设置成功码
    auto email = src_root["email"].asString();
    auto name = src_root["user"].asString();
    auto pwd = src_root["passwd"].asString();
    std::uint64_t user_id;
    auto is_email=src_root["is_email"].asInt();
    user_info users;
    if(!is_email)//uid登录
    {
      user_id=std::atoi(src_root["uid"].asString().c_str());
      users={ user_id, email, pwd};
    }
    else//邮箱登录
        users={std::uint64_t{},email,pwd};
    auto reuslt = MySqlMgr::getInstance()->LoginUser(pwd, users,is_email); // 调用MySqlMgr登录用户
    if (reuslt == ErrorCodes::MySqlError) {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR, "Failed to login user: " + email));
      root["error"] = ErrorCodes::MySqlError; // 设置错误码
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      return true;
    }
    if (reuslt == -1) // 密码错误
    {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR, "Failed to login user: " + name));
      root["error"] = ErrorCodes::UserPasswdError; // 设置错误码
      root["uid"] = users.uid;
      std::string jsonstr = root.toStyledString();
      std::cout << jsonstr << std::endl;
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      return true;
    }
    if (reuslt == -2) // 用户不存在
    {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR, "Failed to login user: " + name));
      root["error"] = ErrorCodes::UserNotExist; // 设置错误码
      root["uid"] = users.uid;
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      return true;
    }
    auto stub_m =
        std::move(gRPCServerPool::getInstance()->get_stub()); // 获取gRPC链接
    auto client_context = grpc::ClientContext();
    message::GetChatServerReq grpc_request;
    grpc_request.set_uid(users.uid);
    message::GetChatServerRsp grpc_respon;
    grpc::Status Status_m =
        stub_m->GetChatServer(&client_context, grpc_request, &grpc_respon);
    if (Status_m.ok()) {
      root["error"] = ErrorCodes::Success; // 设置成功码
      root["uid"] = users.uid;
      root["token"] = grpc_respon.token();
      root["host"] = grpc_respon.host();
      root["port"] = grpc_respon.port();
      gRPCServerPool::getInstance()->release_stub(
          std::move(stub_m)); // 归还gRPC链接
      auto jsonstr = root.toStyledString();
      std::cout << jsonstr << std::endl;
      beast::ostream(connection->response_.body()) << jsonstr; // 输出JSON响应
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::INFO, "User login successfully: " + name));
      return true;
    } else {
      std::cout << Status_m.error_message() << std::endl;
      gRPCServerPool::getInstance()->release_stub(
          std::move(stub_m)); // 归还gRPC链接
      root["error"] = ErrorCodes::RPCFail;
      root["uid"] = users.uid;
      auto jsonstr = root.toStyledString();
      std::cout << jsonstr << std::endl;
      beast::ostream(connection->response_.body()) << jsonstr; // 输出JSON响应
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::INFO, "User login successfully: " + name));
      return true;
    }
  });

  reg_post(
      "/get_varify_code_forget_passwd",
      [](std::shared_ptr<HttpConnection> // 获取忘记密码验证码服务
             connection) {
        auto body_str =
            beast::buffers_to_string(connection->request_.body().data());
        std::cout << "Received POST request with body: " << body_str
                  << std::endl;
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root); // 解析JSON数据
        if (!parse_success) {
          LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
              LogLevel::ERROR,
              "Failed to parse JSON request body: " + body_str));
          root["error"] = ErrorCodes::Error_Josn; // 设置错误码
          std::string error_message = root.toStyledString();
          beast::ostream(connection->response_.body())
              << error_message; // 输出错误信息
          return true;
        } else {
          auto MySqlConn = MySqlMgr::getInstance();
          bool select_result = MySqlConn->ExistsUser(
              src_root["email"].asString(),
              src_root["uid"].asString()); // 检查用户是否存在
          if (!select_result) {
            LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
                LogLevel::ERROR,
                "User not exist: " + src_root["email"].asString()));
            root["error"] = ErrorCodes::UserNotExist; // 设置错误码
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body())
                << jsonstr; // 输出错误信息
            return true;
          }
          LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
              LogLevel::INFO,
              "Processing verification code request for email: " +
                  src_root["email"].asString()));
          GetVarifyRsp rsp = VarfiyGrpcClient::getInstance()->GetVarifyCode(
              src_root["email"].asString()); // 调用gRPC客户端获取验证码
          if (rsp.error() == ErrorCodes::Success) {
            LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
                LogLevel::INFO,
                "Verification code sent successfully to email: " +
                    src_root["email"].asString()));
            root["error"] = ErrorCodes::Success; // 设置成功码
            root["email"] = src_root["email"];
            root["uid"] = src_root["uid"];
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->response_.body())
                << jsonstr; // 输出JSON响应
            return true;
          } else {
            LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
                LogLevel::ERROR,
                std::string("Failed to send verification code to email: ") +
                    src_root["email"].asString() +
                    std::string(", error code: ") +
                    std::to_string(rsp.error())));
            root["error"] = rsp.error(); // 设置错误码
            std::string jsonstr = root.toStyledString();
            std::cout << jsonstr << std::endl;
            beast::ostream(connection->response_.body())
                << jsonstr; // 输出错误信息
            return true;
          }
        }
      });

  reg_post("/submit_forget_passwd", [](std::shared_ptr<HttpConnection>
                                           connection) { // 忘记密码提交服务
    auto body_str =
        beast::buffers_to_string(connection->request_.body().data());
    std::cout << "Received POST request with body: " << body_str << std::endl;
    Json::Value root;
    Json::Reader reader;
    Json::Value src_root;
    bool parse_success = reader.parse(body_str, src_root); // 解析JSON数据
    if (!parse_success) {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR, "Failed to parse JSON request body: " + body_str));
      root["error"] = ErrorCodes::Error_Josn; // 设置错误码
      std::string error_message = root.toStyledString();
      beast::ostream(connection->response_.body())
          << error_message; // 输出错误信息
      return true;
    }
    auto email = src_root["email"].asString();
    auto uid = src_root["uid"].asString();
    auto new_passwd = src_root["new_passwd"].asString();
    auto varify_code = src_root["varify_code"].asString();
    auto redis_conn = redispool->getConnection();
    auto b_get_varfiy_code = redis_conn->get("code_" + email);
    if (b_get_varfiy_code->empty()) { // 验证码不存在或过期
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR,
          "The varify code is not exist in redis or expired!"));
      root["error"] = ErrorCodes::VarifyEXpired; // 设置错误码
      std::string jsonstr = root.toStyledString();
      std::cout << jsonstr << std::endl;
      // 输出错误信息
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      return true;
    }
    if (b_get_varfiy_code->compare(varify_code) != 0) { // 验证码不匹配
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR, "The varify code is not match!"));
      root["error"] = ErrorCodes::VarifyError; // 设置错误码
      std::string jsonstr = root.toStyledString();
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      return true;
    }
    // 如果验证码匹配，则继续处理修改密码逻辑
    auto MySqlConn = MySqlMgr::getInstance();
    int64_t result = MySqlConn->Change_Passwd_When_User_Forget_Passwd(
        email, uid,
        new_passwd);  // 修改密码
    if (result == -2) // 用户不存在
    {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR, "User not exist: " + src_root["email"].asString()));
      root["error"] = ErrorCodes::UserNotExist; // 设置错误码
      std::string jsonstr = root.toStyledString();
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      return true;
    }
    if (result == -1) // 修改密码失败
    {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR, "Failed to change password for user: " +
                               src_root["email"].asString()));
      root["error"] = ErrorCodes::MySqlError; // 设置错误码
      std::string jsonstr = root.toStyledString();
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      return true;
    }
    if (result == -3) // 邮箱错误
    {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::ERROR,
          "Email is not match: " + src_root["email"].asString()));
      root["error"] = ErrorCodes::EmailIsNotMatch; // 设置错误码
      std::string jsonstr = root.toStyledString();
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      beast::ostream(connection->response_.body()) << jsonstr; // 输出错误信息
      return true;
    }
    if (result == 1) // 修改密码成功
    {
      LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
          LogLevel::INFO, "Password changed successfully for user: " +
                              src_root["email"].asString()));
      root["error"] = ErrorCodes::Success; // 设置成功码
      root["email"] = src_root["email"];
      root["uid"] = src_root["uid"];
      std::string jsonstr = root.toStyledString();
      redispool->returnConnection(redis_conn);                 // 归还Redis连接
      beast::ostream(connection->response_.body()) << jsonstr; // 输出JSON响应
      return true;
    }
  });

  reg_post("/get_user_icon",
           [](std::shared_ptr<HttpConnection> connection) { // 获取头像
             auto request_str =
                 beast::buffers_to_string(connection->request_.body().data());
             Json::Reader reader;
             Json::Value root;
             Json::Value src;
             if (!reader.parse(request_str, src)) {
               return true;
             } 
              auto is_email=src["is_email"].asInt();
              std::string uid_str;
              std::uint64_t user_id{};
              if(is_email)//如果是邮箱登录
              {
                auto email=src["email"].asString();
                user_id=MySqlMgr::getInstance()->Get_User_id(email);//通过邮箱查询id
              }
              else{
                uid_str = src["uid"].asString();
                root["uid"] = uid_str;
                user_id=std::atoi(uid_str.c_str());
              }
               std::cout << "user id is " << uid_str << std::endl;
               auto user_icon_data = MySqlMgr::getInstance()->Get_User_Icon(user_id);
               if (!user_icon_data.empty()) {
                 root["error"] = ErrorCodes::Success;
                 // 计算base64编码后长度
                 std::string user_icon_data_str;
                 user_icon_data_str.resize(
                     boost::beast::detail::base64::encoded_size(
                         user_icon_data.size()));
                 boost::beast::detail::base64::encode(
                     &user_icon_data_str[0], // 写入目标
                     user_icon_data.data(),  // 源数据
                     user_icon_data.size());
                 root["user_icon"] = user_icon_data_str;
                 auto json_str = root.toStyledString();
                 beast::ostream(connection->response_.body()) << json_str;
               } else {
                 root["error"] = ErrorCodes::MySqlError;
                 root["user_icon"] = "NULL";
                 auto json_str = root.toStyledString();
                 beast::ostream(connection->response_.body()) << json_str;
               }
             }
           );
}

bool LogicalSystem::handle_get(
    std::string path, std::shared_ptr<HttpConnection> con) { // 处理GET请求
  if (get_handlers.find(path) == get_handlers.end()) {
    LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
        LogLevel::WARN, "GET request to unregistered path: " + path));
    return false;
  }
  // 如果找到了对应的处理器，则调用它
  LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
      LogLevel::DEBUG, "Handling GET request for path: " + path));
  get_handlers[path](con);
  return true;
}

LogicalSystem::~LogicalSystem() { // 析构函数清理逻辑系统
  // 清理逻辑系统
  get_handlers.clear();
  post_handlers.clear();
}

void LogicalSystem::reg_post(std::string url, HttpHandler handler) {
  post_handlers.insert(std::make_pair(url, handler));
}

bool LogicalSystem::handle_post(std::string url,
                                std::shared_ptr<HttpConnection> con) {
  if (post_handlers.find(url) == post_handlers.end()) {
    LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
        LogLevel::WARN, "POST request to unregistered URL: " + url));
    LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
        LogLevel::INFO,
        "Request IP:" +
            con->get_socket().remote_endpoint().address().to_v4().to_string()));
    return false;
  } //// 如果没有找到对应的处理器，则返回
  LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
      LogLevel::DEBUG, "Handling POST request for URL: " + url));
  LogQueue::getInstance()->push(std::make_pair<LogLevel, std::string>(
      LogLevel::INFO,
      "Request IP:" +
          con->get_socket().remote_endpoint().address().to_v4().to_string()));
  post_handlers[url](con); // 调用对应的处理器
  return true;             // 返回处理成功
}
