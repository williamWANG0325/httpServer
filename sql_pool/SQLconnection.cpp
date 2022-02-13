/*
 * @Author       : william
 * @Date         : 2022-02-13
*/
#include "SQLconnection.h"

SQLconnection::SQLconnection() {
    connection = mysql_init(nullptr);
}

SQLconnection::~SQLconnection() {
    if(connection) {
        mysql_close(connection);
    }
}


bool SQLconnection::connect (std::string ip, unsigned short port, std::string userName, std::string password, std::string dbName) {
    MYSQL* ptr = mysql_real_connect(connection, ip.c_str(), userName.c_str(), password.c_str(), dbName.c_str(), port, nullptr, 0);
    if (!ptr) {
        // 报错
        return 0;
    }else {
        aliveTime = clock();
        return 1;
    }
}

void SQLconnection::refreshAliveTime() {
    aliveTime = clock();
}

int SQLconnection::getAliveTime() {
    return clock() - aliveTime;
}

bool SQLconnection::query(std::string sql) {
    if (mysql_query(connection, sql.c_str())) {
        // log error
        return 0;
    }
    return 1;
}

MYSQL_RES* SQLconnection::getResult() {
    return mysql_use_result(connection);
}
