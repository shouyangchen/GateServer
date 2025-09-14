#ifndef MY_SQL_MGR_H
#define MY_SQL_MGR_H

#include "MySqlDao.h"
#include "singleton.h"
#include <cstdint>

class MySqlMgr : public singleton<MySqlMgr> {
    friend class singleton<MySqlMgr>;
public:
    ~MySqlMgr() {
    }
    int64_t RegUser(const std::string& name, const std::string& email, const std::string& pwd) {
        return dao.RegUser(name, email, pwd);
    }

    int64_t LoginUser(const std::string& email, const std::string& pwd,user_info&user) {
        return dao.LoginUser(email, pwd,user);
    }

    bool ExistsUser(const std::string& email, const std::string& uid) {
        return dao.ExistsUser(email, uid);
    }

    int64_t Change_Passwd_When_User_Forget_Passwd(const std::string& email, const std::string& uid, const std::string& new_passwd) {
        return dao.Change_Passwd_When_User_Forget_Passwd(email, uid, new_passwd);
    }
    
private:
  MySqlMgr() {
        
    } 
    MysqlDao dao;
};
#endif // MY_SQL_MGR_H