
#ifndef MYSQL_DAO_H
#define MYSQL_DAO_H
#include "MySqlPool.h"
#include <exception>
#include <mysql++/manip.h>
#include <mysql++/mysql++.h>
#include <mysql++/mystring.h>
#include <mysql++/query.h>
#include <mysql++/result.h>
#include <ostream>
#include <vector>
#include "const.h"
#include "ConfiMgr.h"

class MysqlDao
{
public:
    MysqlDao( );
    ~MysqlDao( );
    int64_t RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    int64_t LoginUser(const std::string& pwd, user_info& user,int is_email);
    bool ExistsUser(const std::string& email, const std::string& uid);
    int64_t Change_Passwd_When_User_Forget_Passwd(const std::string& email,const std::string& uid, const std::string& new_passwd);
    std::vector<char> Get_user_icon(int& uid);
    std::uint64_t Get_user_uid(std::string  &user_email);
private:
    std::unique_ptr<MySqlPool> pool_;
};

inline MysqlDao::MysqlDao( )
{
    auto& cfg = ConfiMgr::getInstance( );
    const auto& host = cfg["Mysql"]["host"];
    const auto& port = cfg["Mysql"]["port"];
    const auto& pwd = cfg["Mysql"]["passwd"];
    const auto& schema = cfg["Mysql"]["database"];
    const auto& user = cfg["Mysql"]["user"];
    pool_.reset(new MySqlPool(host, user, pwd, schema, 5));
}

inline MysqlDao::~MysqlDao( ) {
    pool_->close( );
}

inline int64_t MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    try {
        auto conn = pool_->getConnection( );
        if (!conn) {// 如果获取连接失败
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to get MySQL connection"));
            return ErrorCodes::RPCFail;
        }

        mysqlpp::Query query = conn->query( );
        query << "CALL reg_user("
            << mysqlpp::quote << name << ","
            << mysqlpp::quote << pwd << ","
            << mysqlpp::quote << email << ","
            << "@result);"; // 使用 @result 变量来获取存储过程的返回�?
        if (!query.execute( )) {
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to register user: " + name));
            pool_->releaseConnection(conn); // 释放连接
            return ErrorCodes::MySqlError;
        }

        // 获取 @result 的�?
        mysqlpp::Query resultQuery = conn->query("SELECT @result");
        mysqlpp::StoreQueryResult res = resultQuery.store( );
        if (res && res.num_rows( ) > 0) {
            if (res[0][0].is_null( ))
            {
                std::cerr << "Wrong in get res" << std::endl;
            }
            auto result = std::atoi(res[0][0].data( ));
            std::cout << result << std::endl;
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::INFO, "User register result: " + std::to_string(result)));
            pool_->releaseConnection(conn); // 释放连接
            return result;
        }
        else {
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to get @result for user: " + name));
            pool_->releaseConnection(conn); // 释放连接
            return ErrorCodes::RPCFail;
        }
    }
    catch (const std::exception& e) {
        LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Exception in RegUser: " + std::string(e.what( ))));
        return ErrorCodes::RPCFail;
    }
}


inline int64_t  MysqlDao::LoginUser(const std::string& pwd, user_info& user,int is_email)
{
    try {
        auto conn = this->pool_->getConnection( );
        if (!conn)
        {
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to get MySQL connection"));
            return ErrorCodes::MySqlError;
        }
        else {
            mysqlpp::Query query = conn->query( );
            query << "CALL Login_user("
                << mysqlpp::quote << user.email << ","
                << mysqlpp::quote << pwd << ","
                <<mysqlpp::quote<<user.uid<<","
                <<mysqlpp::quote<<is_email<<","
                <<"@login_result"<<")"<<";";
            std::cout << "Login_user query: " << query.str( ) << std::endl;
            if (!query.execute( ))
            {
                LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to login user: " +user.email));
                pool_->releaseConnection(conn); // 释放连接
                return ErrorCodes::MySqlError;
            }
            // 获取 @login_result 的值
            mysqlpp::Query resultQuery = conn->query("SELECT @login_result");
            mysqlpp::StoreQueryResult res = resultQuery.store( );
            if (res && res.num_rows( ) > 0)
            {
                auto result = std::atoi(res[0][0].data( ));
                mysqlpp::Query uidQuery = conn->query("SELECT @USER_UID");// 获取用户uid
                mysqlpp::StoreQueryResult uid = uidQuery.store( );
                if (uid && uid.num_rows( ) > 0)
                    user.uid = std::atoi(uid[0][0].data( ));
                LogQueue::getInstance( )->push(std::make_pair(LogLevel::INFO, "User login result: " + std::to_string(result)));
                pool_->releaseConnection(conn); // 释放连接
                return result;
            }
        }
    }
    catch (std::exception& e)
    {
        LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Exception in RegUser: " + std::string(e.what( ))));
        return ErrorCodes::MySqlError;
    }
}

inline bool MysqlDao::ExistsUser(const std::string& email, const std::string& uid)// 检查用户是否存�?
{
    try {
        auto conn = this->pool_->getConnection( );
        if (!conn) {
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to get MySQL connection"));
            return false;
        }

        mysqlpp::Query query = conn->query( );
        query << "SELECT uid FROM user WHERE email = " << mysqlpp::quote << email;
        mysqlpp::StoreQueryResult res = query.store( );// 执�?�查询并获取结果
        if (res && res.num_rows( ) > 0) {
            std::string uid_value;
            res[0]["uid"].to_string(uid_value); // 获取查�?�结果中的uid
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::INFO, "Checking user existence for email: " + email));
            if (uid_value == uid) {
                LogQueue::getInstance( )->push(std::make_pair(LogLevel::INFO, "User exists: " + email));
                pool_->releaseConnection(conn); // 释放连接
                return true; // 用户存在    
            }
            else {
                LogQueue::getInstance( )->push(std::make_pair(LogLevel::INFO, "User does not exist: " + email));
                pool_->releaseConnection(conn); // 释放连接
                return false; // 用户不存�?
            }
        }

    }
    catch (const std::exception& e) {
        LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Exception in ExistsUser: " + std::string(e.what( ))));
        return false;

    }
}


inline int64_t MysqlDao::Change_Passwd_When_User_Forget_Passwd(const std::string& email, const std::string& uid, const std::string& new_passwd)//�?改密码当用户忘�?�密码时
{
    try {
        auto conn = pool_->getConnection( );
        if (!conn) {
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to get MySQL connection"));
            return ErrorCodes::MySqlError;
        }
        mysqlpp::Query query = conn->query( );
        query << "CALL change_passwd_when_user_forget_passwd("
            << mysqlpp::quote << email << ","
            << mysqlpp::quote << uid << ","
            << mysqlpp::quote << new_passwd << ","
            << "@isok);"; // 使用 @result 变量来获取存储过程的返回�?

        if (!query.execute( )) {
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to change password for user: " + email));
            pool_->releaseConnection(conn); // 释放连接
            return ErrorCodes::MySqlError;
        }
        // 获取 @isok 的�?
        mysqlpp::Query resultQuery = conn->query("SELECT @isok");
        mysqlpp::StoreQueryResult res = resultQuery.store( );// 执�?�查询并获取结果

        if (res && res.num_rows( ) > 0) {
            auto result = std::atoi(res[0][0].data( ));
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::INFO, "Password change result for user: " + email + " is " + std::to_string(result)));
            pool_->releaseConnection(conn);
            return result;
        }
        else {
            LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Failed to get @result for user: " + email));
            pool_->releaseConnection(conn);
            return ErrorCodes::RPCFail;
        }
    }
    catch (const std::exception& e) {
        LogQueue::getInstance( )->push(std::make_pair(LogLevel::ERROR, "Exception in Change_Passwd_When_User_Forget_Passwd: " + std::string(e.what( ))));
        return ErrorCodes::RPCFail;
    }
}


inline std::vector<char> MysqlDao::Get_user_icon(int& uid)
{
    std::vector<char> icon_data;
    try {
        auto conn = pool_->getConnection();
        if (!conn) {
            LogQueue::getInstance()->push(std::make_pair(LogLevel::ERROR, "Failed to get MySQL connection"));
            return icon_data;
        }
        mysqlpp::Query query = conn->query();
        query << "SELECT user_icon.picture FROM user_icon WHERE user_icon.user_id=" << mysqlpp::quote << uid << ";";
        std::cout << "SQL: " << query.str() << std::endl;
        mysqlpp::StoreQueryResult result = query.store();
        if (result && result.num_rows() > 0) {
            mysqlpp::sql_blob user_icon_bolb = result[0][0];
            std::cout << "user_icon_bolb.size() = " << user_icon_bolb.size() << std::endl; // 调试用
            if (!user_icon_bolb.empty()) {
                icon_data.assign(user_icon_bolb.data(), user_icon_bolb.data() + user_icon_bolb.size());
            }
        }
        pool_->releaseConnection(conn); // 统一释放连接
    }
    catch (std::exception& e) {
        std::cout << "Exception in Get_user_icon: " << e.what() << std::endl;
    }
    return icon_data;
}


inline std::uint64_t MysqlDao::Get_user_uid(std::string  &user_email)
{
    try{
    auto conn=this->pool_->getConnection();
    if (!conn) {
        LogQueue::getInstance()->push(std::make_pair(LogLevel::ERROR, "Failed to get MySQL connection"));
        return {};
        }
    mysqlpp::Query query=conn->query();
    query<<"SELECT get_user_uid("<<mysqlpp::quote<<user_email<<");";
    std::cout << "SQL: " << query.str() << std::endl;
    mysqlpp::StoreQueryResult result=query.store();
    if(result&&result[0][0])
    {
        auto user_id=std::atol(result[0][0].c_str());
        std::cout<<"user id is "<<user_id<<std::endl;
        pool_->releaseConnection(conn);
        return user_id;
    }
    else{
        pool_->releaseConnection(conn);
        return {};
    }
    }
    catch(std::exception const& e)
    {
        std::cout<<e.what()<<std::endl;
    }
}

#endif // MYSQL_DAO_H