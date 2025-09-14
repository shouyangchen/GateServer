//
// Created by chenshouyang on 25-6-24.
//

#ifndef LOGICALSYSTEM_H
#define LOGICALSYSTEM_H
#include "const.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
class LogicalSystem :public singleton<LogicalSystem> {
    friend class singleton<LogicalSystem>;
public:
    ~LogicalSystem();
    void reg_post(std::string url, HttpHandler handler);// 注册处理POST请求的处理器
    bool handle_get(std::string path,std::shared_ptr<HttpConnection> con);// 处理GET请求
    void reg_get(std::string, HttpHandler handler);// 注册处理GET请求的处理器
    bool handle_post(std::string path,std::shared_ptr<HttpConnection> con);
private:
    LogicalSystem();
    std::map<std::string, HttpHandler> post_handlers; // 存储处理GET请求的映射
    std::map<std::string, HttpHandler> get_handlers; // 存储处理POST请求的映射


};



#endif //LOGICALSYSTEM_H
