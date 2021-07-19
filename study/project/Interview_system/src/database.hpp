#pragma once

#include <stdio.h>
#include <unistd.h>
#include <string>
#include "tools.hpp"
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>

#define JUDGEUSER "select * from reg_userinfo where email='%s'"

#define JUDGEVALUE(p) ((p != NULL) ? p : "")

#define GET_ONE_STU_INFO "select * from stu_info where stu_id=\"%s\""

class DataBaseSer
{
public:
    DataBaseSer(std::string& host, std::string& user, std::string& passwd, std::string& db, uint16_t port)
    //: _host(host), _user(user), _passwd(passwd), _db_name(db), _port(port)
    {
        _host = host;
        _user = user;
        _passwd = passwd;
        _db_name = db;
        _port = port;
        mysql_init(&_mysql);
    }

    ~DataBaseSer()
    {
        mysql_close(&_mysql);
    }

    bool Connect2Mysql()
    {
        //参数：mysql客户端操作句柄、mysql主机ip、用户、密码、数据库名称、端口(3306)、nullptr、xxx
        if (!mysql_real_connect(&_mysql, _host.c_str(), _user.c_str(), _passwd.c_str(), 
                            _db_name.c_str(), _port, NULL, CLIENT_FOUND_ROWS))
        {
            LOG(ERROR, "connect database failed") << mysql_errno(&_mysql) << std::endl;
            return false;
        }
        return true;
    }

    // 插入数据的sql语句
    bool QuerySql(const std::string& sql)
    {
        //设置编码格式
        mysql_query(&_mysql, "set names utf8");

        //插入数据,原理就是使用mysql_query()接口执行传递进来的sql语句
        if (mysql_query(&_mysql, sql.c_str()))  //成功大于0，失败返回0
        {
            LOG(ERROR, "exec sql: ") << sql << "failed!" << std::endl;
            return false;
        }
        return true;

    }

    //查询用户是否存在
    bool QueryUserExist(Json::Value& request_json, Json::Value* result)
    {
        mysql_query(&_mysql, "set names utf8");
        //json对象转换string，需要调用asString接口
        std::string email = request_json["email"].asString();
        std::string password = request_json["password"].asString();

        //1.组织查询语句
        char sql[1024] = {0};
        snprintf(sql, sizeof(sql) - 1, JUDGEUSER, email.c_str());

        //2.查询
        if (mysql_query(&_mysql, sql))
        {
            // 查询语句没有成功
            LOG(ERROR, "database error in query:") << mysql_error(&_mysql) << std::endl;
            return false;
        }

        //获取查询结果， 查询结果称之为结果集,通过返回值返回
        MYSQL_RES* res = mysql_store_result(&_mysql);
        if (!res)
        {
            // 查询语句成功但结果集没有获取成功
            LOG(ERROR, "Get result failed:") << mysql_error(&_mysql) << std::endl;
            return false;
        }
        //2.判断
        //  2.1 判断结果集有没有数据（获取结果集了，但是结果集中没有数据）
        int row_num = mysql_num_rows(res);
        if (row_num <= 0)
        {
            //返回给浏览器的内容是当前用户不存在
            mysql_free_result(res);
            return false;
        }

        //  判断密码是否正确
        MYSQL_ROW row = mysql_fetch_row(res); //获取结果集当中的一行内容
        //返回的row相当于一个数组
        //row[0] --> user_id
        //row[1] --> name
        // ... 以此类推
        //这样获取的方式需要注意一点：当row[x]本身就为NULL，这时强行获取再赋值给string对象，程序就会崩溃
        std::cout << row[2] << std::endl;
        std::string tmp_password = JUDGEVALUE(row[2]);
        if (strcmp(password.c_str(), tmp_password.c_str()) != 0)
        {// 密码是不一致的
            LOG(ERROR, "用户邮箱为：") << email << "登录的密码不对，密码为：" << password << std::endl;
            mysql_free_result(res);
            return false;
        }

        (*result)["stu_id"] = row[0];
        mysql_free_result(res);
        return true;
    }

    bool QueryOneStuInfo(std::string user_id, Json::Value* result)
    {
        mysql_query(&_mysql, "set names utf8");
        //1.组织查询的sql语句
        char sql[1024] = {0};
        snprintf(sql, sizeof(sql) - 1, GET_ONE_STU_INFO, user_id.c_str());
        
        //2.进行查询
        if (mysql_query(&_mysql, sql))
        {
            // 查询语句没有成功
            LOG(ERROR, "database error in query:") << mysql_error(&_mysql) << std::endl;
            return false;
        }

        //3.获取结果集
        MYSQL_RES* res = mysql_store_result(&_mysql);
        if (!res)
        {
            // 查询语句成功但结果集没有获取成功
            LOG(ERROR, "Get result failed:") << mysql_error(&_mysql) << std::endl;
            return false;
        }
        int row_num = mysql_num_rows(res);
        if (row_num <= 0)
        {
            //返回给浏览器的内容是当前用户不存在
            mysql_free_result(res);
            return false;
        }
        //4.打包json串
        MYSQL_ROW row = mysql_fetch_row(res);
        (*result)["stu_id"] = atoi(row[0]);
        (*result)["stu_name"] = JUDGEVALUE(row[1]);
        (*result)["stu_choice_score"] = JUDGEVALUE(row[6]);
        (*result)["stu_program_score"] = JUDGEVALUE(row[7]);
        (*result)["stu_total_score"] = JUDGEVALUE(row[8]);
        (*result)["stu_speculative_score"] = JUDGEVALUE(row[10]);
        (*result)["stu_code_score"] = JUDGEVALUE(row[11]);
        (*result)["stu_think_score"] = JUDGEVALUE(row[12]);
        (*result)["stu_expression_score"] = JUDGEVALUE(row[13]);
        (*result)["stu_interview_score"] = JUDGEVALUE(row[14]);
        (*result)["stu_interview_techer"] = JUDGEVALUE(row[15]);
        (*result)["stu_techer_suggest"] = JUDGEVALUE(row[16]);
        (*result)["stu_interview_time"] = JUDGEVALUE(row[9]);
        
        mysql_free_result(res);
        return true;
    }
private:
    MYSQL _mysql;   //MYSQL就是客户端的操作句柄
    std::string _host;
    std::string _user;
    std::string _passwd;
    std::string _db_name;
    uint16_t _port;
};
