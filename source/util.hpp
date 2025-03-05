#ifndef _M_UTIL_H
#define _M_UTIL_H
#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <fstream>
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>
#include "logger.hpp"

class mysql_util
{
public:
    static MYSQL *mysql_create(const std::string &host,
                               const std::string &username,
                               const std::string &password,
                               const std::string &dbname,
                               uint16_t port = 3306)
    {
        MYSQL *mysql = mysql_init(NULL);
        if (mysql == NULL)
        {
            ELOG("mysql init failed!");
            return nullptr;
        }
        if (mysql_real_connect(mysql, host.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, NULL, 0) == nullptr)
        {
            ELOG("connect mysql server failed : %s", mysql_error(mysql));
            return nullptr;
        }
        if (mysql_set_character_set(mysql, "utf8") != 0)
        {
            ELOG("set client character failed : %s", mysql_error(mysql));
            return nullptr;
        }
        return mysql;
    }
    static bool mysql_exec(MYSQL *mysql, const std::string &sql)
    {
        int ret = mysql_query(mysql, sql.c_str());
        if (ret != 0)
        {
            ELOG("%s", sql.c_str());
            ELOG("mysql query failed : %s", mysql_error(mysql));
            return false;
        }
        return true;
    }
    static void mysql_destroy(MYSQL *mysql)
    {
        if (mysql != nullptr)
        {
            mysql_close(mysql);
        }
    }
};

class json_util
{
public:
    static bool Serialize(const Json::Value &root, std::string *retstr)
    {
        Json::StreamWriterBuilder swb;
        std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
        std::stringstream ss;
        int ret = sw->write(root, &ss);
        if (ret != 0)
        {
            ELOG("Serialize failed!");
            return false;
        }
        *retstr = ss.str();
        return true;
    }
    static bool UnSerialize(const std::string &str, Json::Value *root)
    {
        Json::CharReaderBuilder crb;
        std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
        std::string err; // 用于保存错误信息
        bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), root, &err);
        if (ret == false)
        {
            ELOG("UnSerialize failed!");
            return false;
        }
        return true;
    }
};
class string_util
{
public:
    static int split(const std::string &str, const std::string &sep, std::vector<std::string> *ret)
    {
        //,,...,,123,,,456,789,,,//,是分隔符
        int inx = 0; // 开始查找的位置
        while (inx < str.size())
        {
            size_t pos = str.find(sep, inx);
            if (pos == std::string::npos)
            {
                (*ret).push_back(str.substr(inx));
                break;
            }
            if (inx == pos)
            {
                inx += sep.size();
                continue;
            }
            (*ret).push_back(str.substr(inx, pos - inx));
            inx = pos + sep.size();
        }
        return (*ret).size();
    }
};

class file_util
{
public:
    static bool read(const std::string &filename, std::string *body)
    {
        //打开文件
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs.is_open())
        {
            ELOG("%s file open error!!!", filename.c_str());
            return false;
        }
        //获取文件大小
        size_t fsize = 0;
        ifs.seekg(0, std::ios::end);
        fsize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        body->resize(fsize);
        //将文件中的内容读取出来
        ifs.read(&body->front(), fsize);
        if (ifs.good() == false)
        {
            ELOG("read %s file content error!!!", filename.c_str());
            ifs.close();
            return false;
        }
        //关闭文件
        ifs.close();
        return true;
    }
};

#endif