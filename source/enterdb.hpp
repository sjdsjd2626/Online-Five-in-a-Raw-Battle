#ifndef _M_ENTERDB_H
#define _M_ENTERDB_H
#include <mutex>
#include <cassert>
#include "util.hpp"

class user_table
{
private:
    MYSQL *_mysql;
    std::mutex _mutex;

public:
    user_table(const std::string &host,
               const std::string &username,
               const std::string &password,
               const std::string &dbname,
               uint16_t port = 3306)
    {
        _mysql = mysql_util::mysql_create(host, username, password, dbname, port);
        assert(_mysql != nullptr);
    }
    ~user_table()
    {
        mysql_util::mysql_destroy(_mysql);
        _mysql = nullptr;
    }
    bool insert(Json::Value &user)
    {
        if (user["username"].isNull() || user["password"].isNull()) // 先看外部给的用户信息全不全
        {
            DLOG("user info is broken");
            return false;
        }
#define INSERT_USER "insert into user values(null,'%s',MD5('%s'),1000,0,0,0);"
        char sql[4096] = {0};
        sprintf(sql, INSERT_USER, user["username"].asCString(), user["password"].asCString());
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("insert user info failed");
            return false;
        }
        return true;
    }
    // 登录验证，并返回用户的详细信息
    bool login(Json::Value &user)
    {
        if (user["username"].isNull() || user["password"].isNull()) // 先看外部给的用户信息全不全
        {
            DLOG("user info is broken");
            return false;
        }
#define LOGIN_USER "select id,score,total_count,win_count,lose_count from user where username='%s' and password=MD5('%s');"
        char sql[4096] = {0};
        sprintf(sql, LOGIN_USER, user["username"].asCString(), user["password"].asCString());
        MYSQL_RES *res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("user login failed");
                return false;
            }
            res = mysql_store_result(_mysql);
        }
        int row_num = mysql_num_rows(res);
        if(row_num==0)
        {
            DLOG("user info is not exists,login failed");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = std::stoi(row[0]);
        user["score"] = std::stoi(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        user["lose_count"] = std::stoi(row[4]);
        mysql_free_result(res);
        return true;
    }
    bool select_by_name(const std::string &name, Json::Value &user) // 查询并提取用户信息
    {
#define USER_BY_NAME "select id,username,score,total_count,win_count,lose_count from user where username='%s';"
        char sql[4096] = {0};
        sprintf(sql, USER_BY_NAME, name.c_str());
        MYSQL_RES *res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("select by name failed");
                return false;
            }
            res = mysql_store_result(_mysql);
        }
        int row_num = mysql_num_rows(res);
        if (row_num ==0)
        {
            DLOG("can't find the user by name");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = std::stoi(row[0]);
        user["username"] = row[1];
        user["score"] = std::stoi(row[2]);
        user["total_count"] = std::stoi(row[3]);
        user["win_count"] = std::stoi(row[4]);
        user["lose_count"] = std::stoi(row[5]);
        mysql_free_result(res);
        return true;
    }
    bool select_by_id(int id, Json::Value &user)
    {
#define USER_BY_ID "select id,username,score,total_count,win_count,lose_count from user where id=%d;"
        char sql[4096] = {0};
        sprintf(sql, USER_BY_ID, id);
        MYSQL_RES *res = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("select by id failed");
                return false;
            }
            res = mysql_store_result(_mysql);
        }
        int row_num = mysql_num_rows(res);
        if (row_num ==0)
        {
            DLOG("can't find the user by id");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = std::stoi(row[0]);
        user["username"] = row[1];
        user["score"] = std::stoi(row[2]);
        user["total_count"] = std::stoi(row[3]);
        user["win_count"] = std::stoi(row[4]);
        user["lose_count"] = std::stoi(row[5]);
        mysql_free_result(res);
        return true;
    }
    bool win(int id)
    {
        Json::Value val;
        bool ret = select_by_id(id, val);
        if (ret == false)
        {
            DLOG("can not find the winner by id");
            return false;
        }
#define WIN_USER "update user set score=score+30,total_count=total_count+1,win_count=win_count+1 where id=%d;"
        char sql[4096] = {0};
        sprintf(sql, WIN_USER, id);
        ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("do win operation failed");
            return false;
        }
        return true;
    }
    bool lose(int id)
    {
        Json::Value val;
        bool ret = select_by_id(id, val);
        if (ret == false)
        {
            DLOG("can not find the loser by id");
            return false;
        }
#define LOSE_USER "update user set score=score-30,total_count=total_count+1,lose_count=lose_count+1 where id=%d;"
        char sql[4096] = {0};
        sprintf(sql, LOSE_USER, id);
        ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("do lose operation failed");
            return false;
        }
        return true;
    }
};

#endif