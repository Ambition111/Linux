#include "db.hpp"
#include "httplib.h"

#define WWWROOT "./www"


blog_system::TableUser  *table_user;
blog_system::TableTag   *table_tag;
blog_system::TableBlog  *table_blog;


void InsertUser(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "插入用户" << std::endl;

    // 用户信息在req的body中，是一个json字符串
    // 1.获取到json字符串
    std::string json_str = req.body;

    // 2.将json字符串解析成为Json::Value对象
    Json::Reader reader;    // 用来解析json字符串
    Json::Value root;  // 用来接收解析后的json对象
    bool ret = reader.parse(json_str, root);
    if (!ret)
    {
        std::cout << "Failed to parse the json string when insert user!" << std::endl;
        rep.status = 400;
        return;
    }

    // 3.调用table_user中的Insert接口将用户信息写入数据库
    ret = table_user->Insert(root);
    if (!ret)
    {
        std::cout << "Failed to insert user into database!" << std::endl;
        rep.status = 500;
        return;
    }

    // 4.填充rep响应信息
    rep.status = 200;   // 这一句可以省略，因为默认响应状态码就是200
    return;
}
void DeleteUser(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "删除用户" << std::endl;

    // 1.获取要删除的用户id     /user/id    /user/(\d+)
    int user_id = std::stoi(req.matches[1]);  // matches[0]存放的是整个字符串，剩下的是捕捉到的字符串
    // 2.执行数据库操作
    bool ret = table_user->Delete(user_id);
    if (!ret)
    {
        std::cout << "Failed to delete user from database!" << std::endl;
        rep.status = 500;
        return;
    }
    // 3.填充响应
    rep.status = 200;
    return;
}
void UpdateUser(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "更新用户" << std::endl;

    int user_id = std::stoi(req.matches[1]);

    std::string json_str = req.body;
    Json::Reader reader;
    Json::Value root;
    bool ret = reader.parse(json_str, root);
    if (!ret)
    {
        std::cout << "Failed to parse the json string when update user!" << std::endl;
        rep.status = 400;
        return;
    }

    ret = table_user->Update(user_id, root);
    if (!ret)
    {
        std::cout << "Failed to update user info into database!" << std::endl;
        rep.status = 500;
        return;
    }

    rep.status = 200;
    return;
}
void GetAllUser(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "获取所有用户" << std::endl;

    // 1.从数据库获取到所有用户信息并序列化为一个Json::Value对象
    Json::Value root;
    bool ret = table_user->GetAll(&root);
    if (!ret)
    {
        std::cout << "Failed to get all user info from database!" << std::endl;
        rep.status = 500;
        return;
    }

    std::cout << "get alluser success!" <<std::endl;

    // 2.将Json::Value对象转换为json字符串，然后作为响应正文填充到rep中
    Json::FastWriter writer;
    std::string body;
    body = writer.write(root);

    // 填充
    rep.set_content(body, "application/json");
    return;
}
void GetOneUser(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "获取单个用户" << std::endl;

    int user_id = std::stoi(req.matches[1]);
    Json::Value root;
    bool ret = table_user->GetOne(user_id, &root);
    if (!ret)
    {
        std::cout << "Failed to get one user info from database!" << std::endl;
        rep.status = 500;
        return;
    }

    Json::FastWriter writer;
    std::string body = writer.write(root);
    rep.set_content(body, "application/json");

    return;
}


void InsertTag(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "插入标签" << std::endl;

    std::string json_str = req.body;
    Json::Reader reader;
    Json::Value root;
    bool ret = reader.parse(json_str, root);
    if (!ret)
    {
        std::cout << "Failed to parse the json string when insert tag!" << std::endl;
        rep.status = 400;
        return;
    }

    ret = table_tag->Insert(root);
    if (!ret)
    {
        std::cout << "Failed to insert tag into database!" << std::endl;
        rep.status = 500;
        return;
    }

    rep.status = 200;
    return;
}
void DeleteTag(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "删除标签" << std::endl;

    int tag_id = std::stoi(req.matches[1]);
    bool ret = table_tag->Delete(tag_id);
    if (!ret)
    {
        std::cout << "Failed to delete tag from database!" << std::endl;
        rep.status = 500;
        return;
    }
    //rep.status = 200;
    return;
}
void UpdateTag(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "更新标签" << std::endl;

    int tag_id = std::stoi(req.matches[1]);

    std::string json_str = req.body;
    Json::Value root;
    Json::Reader reader;
    bool ret = reader.parse(json_str, root);    
    if (!ret)
    {
        std::cout << "Failed to parse the json string when update tag!" << std::endl;
        rep.status = 400;
        return;
    }

    ret = table_tag->Update(tag_id, root);
    if (!ret)
    {
        std::cout << "Failed to update tag info into database!" << std::endl;
        rep.status = 500;
        return;
    }
    return;
}
void GetAllTag(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "获取所有标签" << std::endl;

    Json::Value root;
    bool ret = table_tag->GetAll(&root);
    if (!ret)
    {
        std::cout << "Failed to get all tag info from database!" << std::endl;
        rep.status = 500;
        return;
    }

    std::cout << "get alltag success!" <<std::endl;

    Json::FastWriter writer;
    std::string body = writer.write(root);
    
    rep.set_content(body, "application/json");
    return;
}
void GetOneTag(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "获取单个标签" << std::endl;

    int tag_id = std::stoi(req.matches[1]);
    Json::Value root;

    bool ret = table_tag->GetOne(tag_id, &root);
    if (!ret)
    {
        std::cout << "Failed to get one tag info from database!" << std::endl;
        rep.status = 500;
        return;
    }

    Json::FastWriter writer;
    std::string body = writer.write(root);
    rep.set_content(body, "application/json");

    return;
}


void InsertBlog(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "插入博客" << std::endl;

    std::string json_str = req.body;
    Json::Reader reader;
    Json::Value root;
    bool ret = reader.parse(json_str, root);
    if (!ret)
    {
        std::cout << "Failed to parse the json string when inseret blog!" << std::endl;
        rep.status = 400;
        return;
    }
    ret = table_blog->Insert(root);
    if (!ret)
    {
        std::cout << "Failed to insert blog into database!" << std::endl;
        rep.status = 500;
        return;
    }
    rep.status = 200;
    return;
}
void DeleteBlog(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "删除博客" << std::endl;

    int blog_id = std::stoi(req.matches[1]);
    bool ret = table_blog->Delete(blog_id);
    if (!ret)
    {
        std::cout << "Failed to delete blog from database!" << std::endl;
        rep.status = 500;
        return;
    }
    return;
}
void UpdateBlog(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "更新博客" << std::endl;

    int blog_id = std::stoi(req.matches[1]);
    std::string json_str = req.body;
    Json::Reader reader;
    Json::Value root;
    bool ret = reader.parse(json_str, root);
    if (!ret)
    {
        std::cout << "Failed to parse the json string when update blog!" << std::endl;
        rep.status = 400;
        return;
    }

    ret = table_blog->Update(blog_id, root);
    if (!ret)
    {
        std::cout << "Failed to update blog info into database!" << std::endl;
        rep.status = 500;
        return;
    }
    return;
}
void GetAllBlog(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "获取所有博客" << std::endl;

    // blog?tag_id=id 按标签获取博客；blog?user_id=id 按用户获取博客
    Json::Value root;
    // req.has_param(const char* key); 判断url里是否具有某个查询字符串
    if (req.has_param("tag_id"))
    {   // 如果有，则通过key值获取到对应的value
        int tag_id = std::stoi(req.get_param_value("tag_id"));
        bool ret = table_blog->GetFromTag(tag_id, &root);
        if (!ret)
        {
            std::cout << "Failed to get all blog info from database by tag!" << std::endl;
            rep.status = 500;
            return;
        }
    }
    else if (req.has_param("user_id"))
    {
        int user_id = std::stoi(req.get_param_value("user_id"));
        bool ret = table_blog->GetFromUser(user_id, &root);
        if (!ret)
        {
            std::cout << "Failed to get all blog info from database by user!" << std::endl;
            rep.status = 500;
            return;
        }
    }
    else
    {
        bool ret = table_blog->GetAll(&root);
        if (!ret)
        {
            std::cout << "Failed to get all blog info from database!" << std::endl;
            rep.status = 500;
            return;
        }
    }

    std::cout << "get allblog success!" <<std::endl;

    Json::FastWriter writer;
    std::string body = writer.write(root);
    rep.set_content(body, "application/json");
    return;
    
}
void GetOneBlog(const httplib::Request& req, httplib::Response& rep)
{
    std::cout << "获取单个博客" << std::endl;

    int blog_id = std::stoi(req.matches[1]);
    Json::Value root;
    bool ret = table_blog->GetOne(blog_id, &root);
    if (!ret)
    {
        std::cout << "Failed to get one blog info from database!" << std::endl;
        rep.status = 500;
        return;
    }
    
    Json::FastWriter writer;
    std::string body = writer.write(root);
    rep.set_content(body, "application");
    return;
}



int main()
{
    MYSQL* mysql = blog_system::MysqlInit();
    if (mysql == nullptr)
        return -1;
    
    table_user = new blog_system::TableUser(mysql);
    table_tag = new blog_system::TableTag(mysql);
    table_blog = new blog_system::TableBlog(mysql);

//// 1.实例化Server对象
    httplib::Server server;

//// 2.给Server对象注册请求处理路由（针对某个请求添加不同的回调处理函数）
/*设置url中的资源路径的相对根目录*/
    // 好处：一些相对根目录下的静态文件资源，httplib会自动进行读取响应
    server.set_base_dir(WWWROOT);

// 用户类的请求
/*新增用户的请求方法*/
    // R"()"  括号中的数据是原始数据，去除特殊字符的含义
    // 路由注册函数的一个参数可以是一个正则表达式
      //正则表达式：用于匹配符合某种规则或特定格式的字符串
        // user/\d+ 能够匹配user/1 user/2 user/11
        // (\d+) 表示到时候要捕捉括号里的数据 如1 2 11
    server.Post(R"(/user)", InsertUser);

/*删除用户的请求方法*/
    server.Delete(R"(/user/(\d+))", DeleteUser);

/*更新用户的请求方法*/
    server.Put(R"(/user/(\d+))", UpdateUser);

/*查看用户信息的请求方法*/
    server.Get(R"(/user)", GetAllUser);
    server.Get(R"(/user/(\d+))", GetOneUser);


// 标签类的请求
    server.Post(R"(/tag)", InsertTag);
    server.Delete(R"(/tag/(\d+))", DeleteTag);
    server.Put(R"(/tag/(\d+))", UpdateTag);
    server.Get(R"(/tag)", GetAllTag);
    server.Get(R"(/tag/(\d+))", GetOneTag);


// 博客类的请求
    server.Post(R"(/blog)", InsertBlog);
    server.Delete(R"(/blog/(\d+))", DeleteBlog);
    server.Put(R"(/blog/(\d+))", UpdateBlog);
    server.Get(R"(/blog)", GetAllBlog); // get /blog-所有 /blog?tag_id=1 /blog?user_id=1
    server.Get(R"(/blog/(\d+))", GetOneBlog);


//// 3.搭建tcp服务器开始监听
    server.listen("192.168.135.132", 9000);


//// 4.收到连接请求，建立连接，创建一个线程去处理这个连接

    
    blog_system::MysqlRelease(mysql);
    return 0;
}

/*  测试数据库函数的增删改查
void TestUser()
{
    MYSQL* mysql = blog_system::MysqlInit();
    blog_system::TableUser table_user(mysql);

    Json::Value user;
    // 插入数据
    //user["name"] = "彭贺文";
    //table_user.Insert(user);
    //user.clear();
    //user["name"] = "吕布";
    //table_user.Insert(user);

    // 查询数据
    //table_user.GetAll(&user);
    //table_user.GetOne(3, &user);

    // 更新数据
    //user["name"] = "貂蝉";
    //table_user.Update(3, user);

    // 删除数据
    table_user.Delete(1);
    // 将Json::Value转换为json字符串进行打印
    //Json::Reader reader;  // 专门用于json反序列化
    //Json::Writer writer;  // 专门用于json序列化 --- 鸡肋，不能直接使用
    // 应该用 Json::StyledWriter writer
    Json::StyledWriter writer;
    std::cout << writer.write(user) << std::endl;

    blog_system::MysqlRelease(mysql);
}
*/