#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <string>
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>
#include <mutex>

#define MYSQL_HOST      "192.168.135.132"
#define MYSQL_DB_NAME   "blog_system"
#define MYSQL_USER      "Root"
#define MYSQL_PASS      "123456"

namespace blog_system
{

    static std::mutex _mutex;

    // 初始化mysql
    static MYSQL* MysqlInit()
    {
        MYSQL* mysql = nullptr;
        // 1.初始化mysql句柄
        mysql = mysql_init(nullptr);
        if (mysql == nullptr)
        {
            std::cout << "mysql init failed！" << std::endl;
            return nullptr;
        }
        // 2.连接mysql服务器
        if (mysql_real_connect(mysql, MYSQL_HOST, MYSQL_USER, MYSQL_PASS, MYSQL_DB_NAME, 0, nullptr, 0) == nullptr)
        {
            std::cout << mysql_error(mysql) << "：mysql connect failed！" << std::endl;
            mysql_close(mysql);
            return nullptr;
        }
            // 3.0.选择数据库  mysql_select_db() 
        // 3.设置mysql客户端字符编码集
        if (mysql_set_character_set(mysql, "utf8") != 0)
        {
            std::cout << mysql_error(mysql) << "：set character failed！" << std::endl;
            mysql_close(mysql);
            return nullptr;
        }
        if (mysql_query(mysql, "set interactive_timeout=24*3600;") != 0)
        {
            std::cout << "set interactive_timeout failed！" << std::endl;
            mysql_close(mysql);
            return nullptr;
        }
        return mysql;
    }
    // 关闭数据库，释放mysql操作句柄
    static void MysqlRelease(MYSQL* mysql)
    {
        if (mysql)
            mysql_close(mysql);
        return;
    }
    // mysql执行
    static bool MysqlQuery(MYSQL* mysql, const std::string& sql)
    {
        int ret = mysql_query(mysql, sql.c_str());
        if (ret != 0)
        {
            std::cout << mysql_error(mysql) << "：query sql：[" << sql.c_str() << "] error！" << std::endl;
            return false;
        }
        return true;
    }


    // 标签信息表
    class TableTag
    {
    public:
        TableTag(MYSQL* mysql) : _mysql(mysql) {}
        
        // 插入一个新标签
        bool Insert(const Json::Value& tag)
        {
            // 1.组织sql语句
#define INSERT_TAG "insert table_tag values(null, '%s');"
            char tmp[4096] = {0};
            // 不要忘了数据的校验 -- 防备sql注入攻击
            // Json::Value.asCString(),返回C风格的字符串
            // sprintf() 按照指定格式将多个数据组织成为一个字符串放到tmp中
            sprintf(tmp, INSERT_TAG, tag["name"].asCString());

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "Insert failed！" << std::endl;
                return false;
            }
            return true; // 每次先把返回值给上，没有返回值的情况下，默认返回假
        }
    
        // 删除一个标签
        bool Delete(int tag_id)
        {
            // 1.组织sql语句
#define DELETE_TAG "delete from table_tag where id=%d;"
            char tmp[4096] = {0};
            sprintf(tmp, DELETE_TAG, tag_id);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "delete failed！" << std::endl;
                return false;
            }
            return true;
        }
        // 更新标签
        bool Update(int tag_id, const Json::Value& tag)
        {
            // 1.组织sql语句
#define UPDATA_TAG "update table_tag set name='%s' where id='%d';"
            char tmp[4096] = {0};
            sprintf(tmp, UPDATA_TAG, tag["name"].asCString(), tag_id);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "Update failed！" << std::endl;
                return false;
            }
            return true;
        }

        // 查询获取全部标签
        bool GetAll(Json::Value* tags)
        {
            // 1.组织sql语句
#define SELECT_TAG_ALL "select * from table_tag;"

            // 实例化了lock_guard对象，并且直接对mutex进行加锁
            // lock_guard对象是一个局部对象，函数退出时其类的析构函数会自动进行解锁，也就是RAII机制
            std::lock_guard<std::mutex> lck(_mutex);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, SELECT_TAG_ALL);
            if (!ret)
            {
                std::cout << "getAll failed！" << std::endl;
                return false;
            }

    //***** 注意考虑mysql_query和mysql_store_result这两步的线程安全问题*****
            // 3.获取结果集（重要）
            MYSQL_RES* res = mysql_store_result(_mysql); // 结果集空间的首地址

            // 4.遍历结果集
            uint64_t num_rows = mysql_num_rows(res); // 获取结果集中的数据条数，即有多少条数据
            if (num_rows <= 0)
            {
                std::cout << "No tag information！" << std::endl;
                mysql_free_result(res); // 释放结果集
                return false;
            }
            for (int i = 0; i < num_rows; ++i)
            {
                // MYSQL_ROW 是一个字符串二级指针，数组里每个元素保存的是一个char*类型的结果集
                MYSQL_ROW row = mysql_fetch_row(res); //遍历结果集，获取每一行数据
                                                      //res中也会保存当前遍历位置，每次遍历会自动遍历下一行

                // 将获取到的每一行结果集组织成Json串
                Json::Value tag;
                tag["id"] = std::stoi(row[0]);
                tag["name"] = row[1];

                tags->append(tag); // 将获取到的结果集添加到tag里然后出参
            }

            mysql_free_result(res);
            return true;
        }

        // 查询获取指定一个标签
        bool GetOne(int tag_id, Json::Value* tag)
        {
            // 1.组织sql语句
#define SELECT_TAG_ONE "select name from table_tag where id=%d;"
            char tmp[1024] = {0};
            sprintf(tmp, SELECT_TAG_ONE, tag_id);

            std::lock_guard<std::mutex> lck(_mutex);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "getOne failed！" << std::endl;
                return false;
            }

            // 3.获取结果集（重要）
            MYSQL_RES* res = mysql_store_result(_mysql); // 结果集空间的首地址

            // 4.遍历结果集
            uint64_t num_rows = mysql_num_rows(res); // 获取结果集中的数据条数，即有多少条数据
            if (num_rows != 1)
            {
                std::cout << "tag information count error！" << std::endl;
                mysql_free_result(res); // 释放结果集
                return false;
            }
            for (int i = 0; i < num_rows; ++i)
            {
                // MYSQL_ROW 是一个二级指针，数组里每个元素保存的是一个char*类型的结果集
                MYSQL_ROW row = mysql_fetch_row(res); //遍历结果集，获取每一行数据
                                                      //res中也会保存当前遍历位置，每次遍历会自动遍历下一行

                // 将获取到的每一行结果集组织成Json串
                (*tag)["id"] = tag_id;
                (*tag)["name"] = row[0];
            }

            mysql_free_result(res);
            return true;
        }

        ~TableTag()
        {}
    private:
        MYSQL* _mysql;
    };

    // 用户信息表
    class TableUser
    {
    public:
        TableUser(MYSQL* mysql) : _mysql(mysql) {}

        // 新增一个用户
        bool Insert(const Json::Value& user)
        {
            // 1.组织sql语句
#define INSERT_USER "insert table_user values(null, '%s');"
            char tmp[4096] = {0};
            sprintf(tmp, INSERT_USER, user["name"].asCString());

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "Insert failed！" << std::endl;
                return false;
            }
            return true; // 每次先把返回值给上，没有返回值的情况下，默认返回假
        }
    
        // 删除一个用户
        bool Delete(int user_id)
        {
            // 1.组织sql语句
#define DELETE_USER "delete from table_user where id=%d;"
            char tmp[4096] = {0};
            sprintf(tmp, DELETE_USER, user_id);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "delete failed！" << std::endl;
                return false;
            }
            return true;
        }
        // 更新用户
        bool Update(int user_id, const Json::Value& user)
        {
            // 1.组织sql语句
#define UPDATA_USER "update table_user set name='%s' where id=%d;"
            char tmp[4096] = {0};
            sprintf(tmp, UPDATA_USER, user["name"].asCString(), user_id);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (ret == false)
            {
                std::cout << "Update failed！" << std::endl;
                return false;
            }
            return true;
        }

        // 查询获取全部用户
        bool GetAll(Json::Value* users)
        {
            // 1.组织sql语句
#define SELECT_USER_ALL "select * from table_user;"

            std::lock_guard<std::mutex> lck(_mutex);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, SELECT_USER_ALL);
            if (!ret)
            {
                std::cout << "getAll user failed！" << std::endl;
                return false;
            }

            // 3.获取结果集（重要）
            MYSQL_RES* res = mysql_store_result(_mysql); // 结果集空间的首地址

            // 4.遍历结果集
            uint64_t num_rows = mysql_num_rows(res); // 获取结果集中的数据条数，即有多少条数据
            if (num_rows <= 0)
            {
                std::cout << "No user information！" << std::endl;
                mysql_free_result(res); // 释放结果集
                return false;
            }
            for (int i = 0; i < num_rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res); 

                // 将获取到的每一行结果集组织成Json串
                Json::Value user;
                user["id"] = std::stoi(row[0]);
                user["name"] = row[1];

                users->append(user); // 将获取到的结果集添加到user里然后出参
            }

            mysql_free_result(res);
            return true;
        }

        // 查询获取指定一个用户
        bool GetOne(int user_id, Json::Value* user)
        {
            // 1.组织sql语句
#define SELECT_USER_ONE "select name from table_user where id=%d;"
            char tmp[1024] = {0};
            sprintf(tmp, SELECT_USER_ONE, user_id);

            std::lock_guard<std::mutex> lck(_mutex);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "getOne user failed！" << std::endl;
                return false;
            }

            // 3.获取结果集（重要）
            MYSQL_RES* res = mysql_store_result(_mysql); // 结果集空间的首地址

            // 4.遍历结果集
            uint64_t num_rows = mysql_num_rows(res); // 获取结果集中的数据条数，即有多少条数据
            if (num_rows != 1)
            {
                std::cout << "user information count error！" << std::endl;
                mysql_free_result(res); // 释放结果集
                return false;
            }
            for (int i = 0; i < num_rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                // 将获取到的每一行结果集组织成Json串
                (*user)["id"] = user_id;
                (*user)["name"] = row[0];
            }

            mysql_free_result(res);
            return true;
        }
        ~TableUser()
        {}
    private:
        MYSQL* _mysql;
    };

    // 博客信息表
    class TableBlog
    {
    public:
        TableBlog(MYSQL* mysql) : _mysql(mysql) {}

        // 新增博客
        bool Insert(const Json::Value& blog)
        {
            // 博客信息表里的字段有 id,tag_id,user_id,title,content,ctime
#define INSERT_BLOG "insert table_blog values(null, %d, %d, '%s', '%s', now());"
            char tmp[4096] = {0};
            sprintf(tmp, INSERT_BLOG, blog["tag_id"].asInt(), blog["user_id"].asInt(), blog["title"].asCString(), blog["content"].asCString());

            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "Insert failed！" << std::endl;
                return false;
            }
            return true;
        }

        // 删除博客
        bool Delete(int blog_id)
        {
            // 博客信息表里的字段有 id,tag_id,user_id,title,content,ctime
            // 1.组织sql语句
#define DELETE_BLOG "delete from table_blog where id=%d;"
            char tmp[4096] = {0};
            sprintf(tmp, DELETE_BLOG, blog_id);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "Delete failed！" << std::endl;
                return false;
            }
            return true;
        }

        // 更新博客
        bool Update(int blog_id, const Json::Value& blog)
        {
            // 博客信息表里的字段有 id,tag_id,user_id,title,content,ctime
            // 1.组织sql语句
#define UPDATA_BLOG "update table_blog set tag_id=%d, title='%s', content='%s' where id=%d;"
            char tmp[4096] = {0};
            sprintf(tmp, UPDATA_BLOG, blog["tag_id"].asInt(), blog["title"].asCString(), blog["content"].asCString(), blog_id);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "Update failed！" << std::endl;
                return false;
            }
            return true;
        }

        // 查询获取全部博客
        bool GetAll(Json::Value* blogs)
        {
            // 1.组织sql语句
#define SELECT_BLOG_ALL "select * from table_blog;"

            std::lock_guard<std::mutex> lck(_mutex);

            // 2.执行sql语句
            bool ret = MysqlQuery(_mysql, SELECT_BLOG_ALL);
            if (!ret)
            {
                std::cout << "getAll failed！" << std::endl;
                return false;
            }

            // 3.获取结果集（重要）
            MYSQL_RES* res = mysql_store_result(_mysql); // 结果集空间的首地址

            // 4.遍历结果集
            uint64_t num_rows = mysql_num_rows(res); // 获取结果集中的数据条数，即有多少条数据
            if (num_rows <= 0)
            {
                std::cout << "No blog information！" << std::endl;
                mysql_free_result(res); // 释放结果集
                return false;
            }
            for (int i = 0; i < num_rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res); 
                // 将获取到的每一行结果集组织成Json串

                Json::Value blog;
                // id,tag_id,user_id,title,content,ctime
                blog["id"] = std::stoi(row[0]);
                blog["tag_id"] = std::stoi(row[1]);
                blog["user_id"] = std::stoi(row[2]);
                blog["title"] = row[3];
                blog["content"] = row[4];
                blog["ctime"] = row[5];
                blogs->append(blog); // 将获取到的结果集添加到blogs里然后出参
            }

            mysql_free_result(res);
            return true;
        }

        // 查询获取指定标签的博客
        bool GetFromTag(int tag_id, Json::Value* blogs)
        {
            // 1.组织sql语句
#define SELECT_BLOG_TAG "select * from table_blog where tag_id=%d;"
            // 2.执行sql语句
            char tmp[4096] = {0};
            sprintf(tmp, SELECT_BLOG_TAG, tag_id);

            std::lock_guard<std::mutex> lck(_mutex);

            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "getAll failed！" << std::endl;
                return false;
            }

            // 3.获取结果集（重要）
            MYSQL_RES* res = mysql_store_result(_mysql); // 结果集空间的首地址

            // 4.遍历结果集
            uint64_t num_rows = mysql_num_rows(res); // 获取结果集中的数据条数，即有多少条数据
            if (num_rows <= 0)
            {
                std::cout << "No blog information！" << std::endl;
                mysql_free_result(res); // 释放结果集
                return false;
            }
            for (int i = 0; i < num_rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res); 
                // 将获取到的每一行结果集组织成Json串

                Json::Value blog;
                // id,tag_id,user_id,title,content,ctime
                blog["id"] = std::stoi(row[0]);
                blog["tag_id"] = std::stoi(row[1]);
                blog["user_id"] = std::stoi(row[2]);
                blog["title"] = row[3];
                blog["content"] = row[4];
                blog["ctime"] = row[5];
                blogs->append(blog); // 将获取到的结果集添加到blogs里然后出参
            }

            mysql_free_result(res);
            return true;
        }

        // 查询获取指定用户的博客
        bool GetFromUser(int user_id, Json::Value* blogs)
        {
            // 1.组织sql语句
#define SELECT_BLOG_USER "select * from table_blog where user_id=%d;"
            // 2.执行sql语句
            char tmp[4096] = {0};
            sprintf(tmp, SELECT_BLOG_USER, user_id);

            std::lock_guard<std::mutex> lck(_mutex);

            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "getAll failed！" << std::endl;
                return false;
            }

            // 3.获取结果集（重要）
            MYSQL_RES* res = mysql_store_result(_mysql); // 结果集空间的首地址

            // 4.遍历结果集
            uint64_t num_rows = mysql_num_rows(res); // 获取结果集中的数据条数，即有多少条数据
            if (num_rows <= 0)
            {
                std::cout << "No blog information！" << std::endl;
                mysql_free_result(res); // 释放结果集
                return false;
            }
            for (int i = 0; i < num_rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res); 
                // 将获取到的每一行结果集组织成Json串

                Json::Value blog;
                // id,tag_id,user_id,title,content,ctime
                blog["id"] = std::stoi(row[0]);
                blog["tag_id"] = std::stoi(row[1]);
                blog["user_id"] = std::stoi(row[2]);
                blog["title"] = row[3];
                blog["content"] = row[4];
                blog["ctime"] = row[5];
                blogs->append(blog); // 将获取到的结果集添加到blogs里然后出参
            }

            mysql_free_result(res);
            return true;
        }

        // 查询获取指定用户的博客
        bool GetOne(int blog_id, Json::Value* blog)
        {
            // 1.组织sql语句
#define SELECT_BLOG_ONE "select * from table_blog where id=%d;"
            // 2.执行sql语句
            char tmp[4096] = {0};
            sprintf(tmp, SELECT_BLOG_ONE, blog_id);

            std::lock_guard<std::mutex> lck(_mutex);
            
            bool ret = MysqlQuery(_mysql, tmp);
            if (!ret)
            {
                std::cout << "getAll failed！" << std::endl;
                return false;
            }

            // 3.获取结果集（重要）
            MYSQL_RES* res = mysql_store_result(_mysql); // 结果集空间的首地址

            // 4.遍历结果集
            uint64_t num_rows = mysql_num_rows(res); // 获取结果集中的数据条数，即有多少条数据
            if (num_rows <= 0)
            {
                std::cout << "No blog information！" << std::endl;
                mysql_free_result(res); // 释放结果集
                return false;
            }
            for (int i = 0; i < num_rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res); 
                // 将获取到的每一行结果集组织成Json串

                // id,tag_id,user_id,title,content,ctime
                (*blog)["id"] = std::stoi(row[0]);
                (*blog)["tag_id"] = std::stoi(row[1]);
                (*blog)["user_id"] = std::stoi(row[2]);
                (*blog)["title"] = row[3];
                (*blog)["content"] = row[4];
                (*blog)["ctime"] = row[5];
            }

            mysql_free_result(res);
            return true;
        }
        ~TableBlog()
        {}
    private:
        MYSQL* _mysql;
    };
};