#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <unordered_map>


class StringTools
{
public:
    //input:待切割的字符串
    //split_char:分隔符(按什么分隔符进行切割)
    //output:存放切割之后的字符串
    //加上static关键字，就可以用StringTools::Split()直接进行调用。不用再实例化对象
    static void Split(const std::string& input, const std::string& split_char, std::vector<std::string>* output)
    {
        /*split()参数：
            1.存放切割之后的字符串
            2.分隔符
            3.如果遇见多个分隔符，按照一个分割还是按照多个分割
                token_compress_off  有多少个就分割多少个
                token_compress_on   不管有多少个都按一个分割
        */
        boost::split(*output, input, boost::is_any_of(split_char), boost::token_compress_off);;
    }
};

//日志文件
//[时间 日志等级 文件:行号] 具体的日志信息

//日志时间
class LogTime
{
public:
    //获取时间戳：年月日时分秒
    static void GetTimeStamp(std::string* time_stamp)
    {
        time_t sys_time;
        time(&sys_time);    //获取1970-01-01到现在的秒数

        struct tm* st = localtime(&sys_time);
        char buf[30] = {0};
        //格式化输出 年-月-日-时-分-秒
        snprintf(buf, sizeof(buf) - 1, "%04d-%02d-%02d %02d-%02d-%02d", st->tm_year + 1900, 
                 st->tm_mon + 1, st->tm_mday, st->tm_hour, st->tm_min, st->tm_sec);

        time_stamp->assign(buf, strlen(buf));
    }
};

//划分日志等级
const char* Level[] = {
        "INFO", 
        "WARNING", 
        "ERROR", 
        "FATAL", 
        "DEBUG"
};

enum LogLevel
{
        INFO = 0, 
        WARNING, 
        ERROR, 
        FATAL, 
        DEBUG
};

//在当前位置展开，获取当前文件的名称和行号
inline std::ostream& Log(LogLevel lev, const char* file, int line, const std::string& log_msg)
{
    //获取日志等级
    std::string log_level = Level[lev];
    //获取时间戳
    std::string time_stamp;
    LogTime::GetTimeStamp(&time_stamp);
    //组织输出内容
    std::cout << "[" << time_stamp << " " << log_level << " " << file << ":" << line << "]" << " " << log_msg;
    return std::cout;
}

#define LOG(lev, msg) Log(lev, __FILE__, __LINE__, msg)


//分割正文提交的数据
class UrlUtil
{
public:
    //eg:email=2460819991%40qq.com&password=1897449peng
    //key-value形式 采用unorder_map
    static void PraseBody(const std::string& body, std::unordered_map<std::string, std::string>* param)
    {
        std::vector<std::string> output;//存放切割后的字符串

        //第一次切割
        //将body以"&"为切割符切割成email=2460819991%40qq.com和password=1897449peng
        StringTools::Split(body, "&", &output);

        //第二次切割
        for (const auto& e : output)
        {
            std::vector<std::string> kv;
            StringTools::Split(e, "=", &kv);

            if (kv.size() != 2)   //防止恶意提交数据，如只提交账号不提交密码
                continue;

            //将URL解码后的数据存放在unordered_map类型的param并出参
            // 将<kv[0], ""> 即<"email","">插入map中，插入成功，返回value的引用，将kv[1]即"password"赋值给该引用结果
            (*param)[kv[0]] = UrlDecode(kv[1]); //注意要URL解码
        }
    }

private:
    static unsigned char ToHex(unsigned char x) 
    { 
        return  x > 9 ? x + 55 : x + 48; 
    }
    
    static unsigned char FromHex(unsigned char x) 
    { 
        unsigned char y;
        if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
        else if (x >= '0' && x <= '9') y = x - '0';
        else assert(0);
        return y;
    }
    
    //URL编码
    static std::string UrlEncode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (isalnum((unsigned char)str[i]) || 
                (str[i] == '-') ||
                (str[i] == '_') || 
                (str[i] == '.') || 
                (str[i] == '~'))
                strTemp += str[i];
            else if (str[i] == ' ')
                strTemp += "+";
            else
            {
                strTemp += '%';
                strTemp += ToHex((unsigned char)str[i] >> 4);
                strTemp += ToHex((unsigned char)str[i] % 16);
            }
        }
        return strTemp;
    }
    
    //URL解码
    static std::string UrlDecode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (str[i] == '+') strTemp += ' ';
            else if (str[i] == '%')
            {
                assert(i + 2 < length);
                unsigned char high = FromHex((unsigned char)str[++i]);
                unsigned char low = FromHex((unsigned char)str[++i]);
                strTemp += high*16 + low;
            }
            else strTemp += str[i];
        }
        return strTemp;
    }
};













































