#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <jsoncpp/json/json.h>
#include "tools.hpp"
#include "database.hpp"
#include "httplib.h"
#include "session.hpp"


#define REGUSER "insert into reg_userinfo (name, password, email) values(\"%s\", \"%s\", \"%s\")"
#define USERINFO "insert into stu_info (stu_name, stu_school, stu_major, stu_grade, stu_mobile) values(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\")"
#define START_TRANSACTION "start transaction"
#define COMMIT "commit"

#define UPDATE_INTERVIEW_TIME "update stu_info set stu_choice_score=%d, stu_program_score=%d, stu_total_score=%d, stu_interview_time=\"%s\" where stu_id=%d"

class Ais_Ser
{
public:
    Ais_Ser()
    {
        _ser_ip.clear();
        _ser_port = -1;;
        _db = nullptr;
        _db_ip.clear();
        _db_user.clear();
        _db_passwd.clear();
        _db_name.clear();
        _db_port = -1;

        _all_session = nullptr;
    }

    //初始化
    // 重新写一个初始化函数而不直接在构造函数里初始化的原因是：构造函数没有返回值；
    // 如果出现_ser_ip/_ser_port初始化非法的情况，程序需要直接返回一个错误信息告诉程序员。
    int OnInit(const std::string& config_fileanme)
    {
        //1.加载配置文件
        if (!Load(config_fileanme))
        {
            LOG(ERROR, "open config file failed") << std::endl;
            return -1;
        }
        
        //2.初始化数据库模块
        _db = new DataBaseSer(_db_ip, _db_user, _db_passwd, _db_name, _db_port);
        if (!_db)
        {
            LOG(ERROR, "create database failed") << std::endl;
            return -2;
        }

        //3.连接数据库
        if (!_db->Connect2Mysql())
        {
            LOG(ERROR, "connect database failed") << std::endl;
            return -3;
        }

        _all_session = new AllSessionInfo();
        if (!_all_session)
        {
            LOG(ERROR, "create all session info failed") << std::endl;
            return -4;
        }
        return 0;
    }
    
    void Start()
    {
        //注册请求  （用户名字、密码、邮箱）
        _http_ser.Post("/register", [this](const httplib::Request& res, httplib::Response& resp){
            //将解析出来的URL各种数据存放在map
            std::unordered_map<std::string, std::string> parm;
            UrlUtil::PraseBody(res.body, &parm);

            //1.将数据再存放到数据库中
              //1.1 针对注册的信息，先插入注册信息表
            std::string name = parm["name"];
            std::string password = parm["password"];
            std::string email = parm["email"];

            std::string school = parm["school"];
            std::string major = parm["major"];
            std::string class_no = parm["class_no"];
            std::string phone_num = parm["phone_num"];

              //1.2 组织插入语句
            Json::Value response_json;
            Json::FastWriter writer;

            //开启事务
            _db->QuerySql(START_TRANSACTION);

            //创建保存点
            _db->QuerySql("savepoint 1");

              //将name，password，email插入到注册信息表reg_userinfo
            char buf[1024] = {0};
            snprintf(buf, sizeof(buf) - 1, REGUSER, name.c_str(), password.c_str(), email.c_str());
            bool ret = _db->QuerySql(buf);
            if (!ret)
            {
                //1.第一个表就插入失败了
                //  1.1 结束事务
                _db->QuerySql(COMMIT);
                //  1.2 返回应答
                response_json["is_insert"] = false;
                resp.body = writer.write(response_json);
                resp.set_header("Content-Type", "application/json");
                //  1.3 结束这个函数
                return;
            }

              //将school，major，grade等插入到学生信息表stu_info
            memset(buf, '\0', sizeof(buf));
            snprintf(buf, sizeof(buf) - 1, USERINFO, name.c_str(), school.c_str(), major.c_str(), class_no.c_str(), phone_num.c_str());
            ret = _db->QuerySql(buf);
            if (!ret)
            {
                //2.第一个表插入成功，但第二个表插入失败
                //  2.1 回滚
                _db->QuerySql("rollback to 1");
                //  2.2 提交事务
                _db->QuerySql(COMMIT);
                //  2.3 组织应答
                response_json["is_insert"] = false;
                resp.body = writer.write(response_json);
                resp.set_header("Content-Type", "application/json");
                //  2.4 结束函数
                return;
            }
            //提交事务
            _db->QuerySql(COMMIT);

            //2.给浏览器响应一个应答，需要是json格式
            response_json["is_insert"] = true;
            resp.body = writer.write(response_json);
            resp.set_header("Content-Type", "application/json");
        });
        
        //登录请求
        _http_ser.Post("/login", [this](const httplib::Request& res, httplib::Response& resp){
            //1.解析提交的数据
            std::unordered_map<std::string, std::string> parm;
            UrlUtil::PraseBody(res.body, &parm);
            Json::Value request_json;
            request_json["email"] = parm["email"];
            request_json["password"] = parm["password"];
            
            //2.校验用户的邮箱和密码
            //  2.1 如果校验失败，则给浏览器返回false
            //  2.2 如果校验成功，执行下面的第三步
            //具体的操作步骤，需要在注册表中进行查询，用提交上来的邮箱作为查询的依据
            //  如果邮箱不存在，则登录失败
            //  如果邮箱存在
            //     密码正确，则登录成功
            //     密码错误，则登录失败

            Json::Value response_json;
            
            bool ret = _db->QueryUserExist(request_json, &response_json);
            if (!ret)
            {
                response_json["login_status"] = false;
            }
            else
            {
                response_json["login_status"] = true;
            }
            

            //3.返回sessionid，用于标识当前用户。前提是在登录正常的情况下
            //  3.1 获取指定用户的信息
            Json::Value user_info;
            _db->QueryOneStuInfo(response_json["stu_id"].asString(), &user_info);
            
            //  3.2 生成session_id
            Session sess(user_info);
            std::string session_id = sess.GetSessionId();
            _all_session->SetSessionValue(session_id, sess);

            std::string tmp = "JSESSIONID=";
            tmp += session_id;

            Json::FastWriter writer;
            resp.body = writer.write(response_json);

            //将session_id放到cookie中返回给浏览器
            resp.set_header("Set-Cookie", tmp.c_str());
            resp.set_header("Content-Type", "application/json");
        });

        //面试预约界面请求
        _http_ser.Get("/interview", [this](const httplib::Request& res, httplib::Response& resp){
            //1.根据请求头部的session_id，从_all_session查询到对应的用户信息
            std::string session_id;
            GetSessionId(res, &session_id);

            Session sess;
            bool ret = _all_session->GetSessionValue(session_id, &sess);
            if (!ret)
            {
                //302 重定向
                resp.set_redirect("/index.html");
                return;
            }
           
            //2.再去查询数据库，获取用户信息
            Json::Value response_json;
            ret = _db->QueryOneStuInfo(sess._user_info["stu_id"].asString(), &response_json);
            if (!ret)
            {
               return;
            }

            //3.组织应答
            Json::FastWriter writer;
            resp.body = writer.write(response_json);
            resp.set_header("Content-Type", "application/json");
        });
        _http_ser.Post("/post_interview", [this](const httplib::Request& res, httplib::Response& resp){
                //1.从header获取sessionid
                std::string session_id;
                GetSessionId(res, &session_id);

                Session sess;
                _all_session->GetSessionValue(session_id, &sess);

                //2.获取正文中那个提交的信息，进行切割，获取到valus
                std::unordered_map<std::string, std::string> parm;
                parm.clear();
                UrlUtil::PraseBody(res.body, &parm);
                std::string choice_score = parm["choice_score"];
                std::string program_score = parm["program_score"];
                std::string total_score = parm["total_score"];
                std::string interview_time = parm["interview_time"];

                //3.组织更新的sql语句
                char sql[1024] = {0};
                snprintf(sql, sizeof(sql) - 1, UPDATE_INTERVIEW_TIME, atoi(choice_score.c_str()), atoi(program_score.c_str()),
                         atoi(total_score.c_str()), interview_time.c_str(), sess._user_info["stu_id"].asInt());
                
                //4.调用执行sql语句的函数
                Json::Value response_json;
                bool ret = _db->QuerySql(sql);
                if (!ret)
                {
                    response_json["is_modify"] = false;
                }
                else
                {
                    response_json["is_modify"] = true;
                }
            
                //5.组织应答
                Json::FastWriter writer;
                resp.body = writer.write(response_json);
                resp.set_header("Content-Type", "application/json");
            });


        //提交面试预约数据请求
        
        //侦听
        _http_ser.set_mount_point("/", "./www");
        LOG(INFO, "start server... listen ip:") << _ser_ip << " listen port:" << _ser_port << std::endl;
        _http_ser.listen(_ser_ip.c_str(), _ser_port);
    }

    void GetSessionId(httplib::Request res, std::string* session_id)
    {
        std::string session = res.get_header_value("Cookie");
        //JSESSIONID=9fa58768e6012732fec11a11971b37c1
        std::unordered_map<std::string, std::string> parm;
        UrlUtil::PraseBody(session, &parm);
        *session_id = parm["JSESSIONID"];
    }
    //加载配置文件（ip和port），之后会在OnInit里调用
    bool Load(const std::string& config_filename)
    {
        std::ifstream file(config_filename.c_str());
        if (!file.is_open())//文件没有打开
        {
            //test LOG(ERROR, "open file failed") << std::endl;
            return false;
        }
        //文件正常打开
        
        std::string line;
        std::vector<std::string> output;    //保存每次读到的那一行数据
        while (std::getline(file, line))   //从文件中获取一行到line中
        {
            output.clear();     //由于每次切割后output里还存有上一次切割的结果，所以每次切割前需要清空
            //切割
            StringTools::Split(line, "=", &output);

            //解析内容
            if (strcmp(output[0].c_str(), "ser_ip") == 0)
            {
                if (output[1].empty())
                {
                    LOG(ERROR, "ser_ip is empty") << std::endl;
                    return false;
                }
                _ser_ip = output[1];
            }
            else if (strcmp(output[0].c_str(), "ser_port") == 0)
            {
                if (output[1].empty())
                {
                    LOG(ERROR, "ser_port is empty") << std::endl;
                    return false;
                }
                _ser_port = atoi(output[1].c_str());
            }
            else if (strcmp(output[0].c_str(), "db_ip") == 0)
            {
                if (output[1].empty())
                {
                    LOG(ERROR, "db_ip is empty") << std::endl;
                    return false;
                }
                _db_ip = output[1];
            }
            else if (strcmp(output[0].c_str(), "db_user") == 0)
            {
                if (output[1].empty())
                {
                    LOG(ERROR, "db_user is empty") << std::endl;
                    return false;
                }
                _db_user = output[1];
            }
            else if (strcmp(output[0].c_str(), "db_passwd") == 0)
            {
                if (output[1].empty())
                {
                    LOG(ERROR, "db_passwd is empty") << std::endl;
                    return false;
                }
                _db_passwd = output[1];
            }
            else if (strcmp(output[0].c_str(), "db_name") == 0)
            {
                if (output[1].empty())
                {
                    LOG(ERROR, "db_name is empty") << std::endl;
                    return false;
                }
                _db_name = output[1];
            }
            else if (strcmp(output[0].c_str(), "db_port") == 0)
            {
                if (output[1].empty())
                {
                    LOG(ERROR, "db_port is empty") << std::endl;
                    return false;
                }
                _db_port = atoi(output[1].c_str());
            }
        }
        return true;
    }
private:
    std::string _ser_ip;     //服务端侦听的ip地址
    uint16_t _ser_port;   //服务端侦听的端口

    DataBaseSer* _db;
    std::string _db_ip;
    std::string _db_user;
    std::string _db_passwd;
    std::string _db_name;
    uint16_t _db_port;

    httplib::Server _http_ser;

    //所有登录用户的会话信息
    AllSessionInfo* _all_session;
};

int main(int argc, char* argv[])
{
    Ais_Ser as;
    int ret = as.OnInit("./config_ais.cfg");
    if (ret < 0)
    {
        LOG(ERROR, "Init server failed") << std::endl;
        return -1;
    }

    as.Start();

    return 0;
}
