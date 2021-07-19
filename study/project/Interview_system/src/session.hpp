#pragma once

#include <string>
#include <jsoncpp/json/json.h>
#include <openssl/md5.h>
#include "tools.hpp"
#include <unordered_map>
#include <pthread.h>


//1.计算sessionid
//2.保存登录用户sessionid和其对应的用户信息

class Session
{
public:
    Session()
    {}
    ~Session()
    {}

    Session(Json::Value& user_info)
    {
        _origin_str.clear();
        _user_info = user_info;

        //计算下原始串 stu_id，stu_name，stu_intertview_time
        _origin_str += std::to_string(_user_info["stu_id"].asInt());
        _origin_str += _user_info["stu_name"].asString();
        _origin_str += _user_info["stu_interview_time"].asString();
    }

    bool SumMd5()
    {
        //生成32位的Md5值，当做session_id
        //1.定义Md5操作句柄&进行初始化
        MD5_CTX ctx;
        MD5_Init(&ctx);

        //2.计算Md5值
        int ret = MD5_Update(&ctx, _origin_str.c_str(), _origin_str.size());
        if (!ret)
        {
            LOG(ERROR, "MD5_Update failed") << std::endl;
            return false;
        }
        //3.获取计算完成的Md5值
        unsigned char md5[16] = {0};
        ret = MD5_Final(md5, &ctx);
        if (ret != 1)
        {
            LOG(ERROR, "MD5_Final failed") << std::endl;
            return false;
        }

        //32位的字符串就是计算出来的session_id
        char buf[32] = {0};
        char tmp[3] = {0};
        for (int i = 0; i < 16; i++)
        {
            sprintf(tmp, "%02x", md5[i]);
            strncat(buf, tmp, 2);
        }

        LOG(INFO, buf) << std::endl;
        _sesson_id = buf;
        return true;
    }

    std::string& GetSessionId()
    {
        SumMd5();
        return _sesson_id;
    }
//private:
    //保存的session_id
    std::string _sesson_id;
    //原始串，用来生成session_id
    std::string _origin_str;
    //生成原始串的内容，stu_id，stu_name，stu_intertview_time
    Json::Value _user_info;
};

class AllSessionInfo
{
public:
    AllSessionInfo()
    {
        _session_map.clear();
        pthread_mutex_init(&_map_lock, NULL);
    }
    ~AllSessionInfo()
    {
        _session_map.clear();
        pthread_mutex_destroy(&_map_lock);
    }

    bool SetSessionValue(std::string& session_id, Session& session_info)
    {
        pthread_mutex_lock(&_map_lock);
        _session_map.insert(std::make_pair(session_id, session_info));
        pthread_mutex_unlock(&_map_lock);
        return true;
    }
    bool GetSessionValue(std::string& session_id, Session* session_info)
    {
        pthread_mutex_lock(&_map_lock);
        auto iter = _session_map.find(session_id);
        if (iter == _session_map.end())
        {
            pthread_mutex_unlock(&_map_lock);
            return false;
        }

        *session_info = iter->second;
        pthread_mutex_unlock(&_map_lock);
        return true;
    }
private:
    //key: _session_id
    //value: Session
    std::unordered_map<std::string, Session> _session_map;
    pthread_mutex_t _map_lock;
};
