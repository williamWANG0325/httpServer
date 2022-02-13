/*
 * @Author       : william
 * @Date         : 2022-02-13
*/
#pragma once

#include <ctime>
#include <mysql/mysql.h>
#include <string>

class SQLconnection
{
public:
    SQLconnection() ;
    ~SQLconnection() ;
    bool connect(std::string ip, unsigned short port, std::string userName, std::string password, std::string dbName);
    void refreshAliveTime();
    int getAliveTime();
    bool query(std::string);
    MYSQL_RES * getResult();

private:
    MYSQL* connection;
    clock_t aliveTime;
};

