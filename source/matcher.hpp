#ifndef _M_MATCHER_H
#define _M_MATCHER_H
#include <list>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "room.hpp"
#include "manageOnlineGamer.hpp"
#include "enterdb.hpp"
#include "util.hpp"
template <class T>
class match_queue
{
private:
    // 双向链表，因为我们有删除中间数据的需要
    std::list<T> _list;
    std::mutex _mutex;
    std::condition_variable _cond;

public:
    int size()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _list.size();
    }
    bool empty()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _list.empty();
    }
    // 阻塞线程
    void wait()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock);
    }
    // 入队数据，并唤醒线程
    void push(const T &data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _list.push_back(data);
        _cond.notify_all();
    }
    bool pop(T &data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_list.empty() == true)
        {
            return false;
        }
        data = _list.front();
        _list.pop_front();
        return true;
    }
    // 移除指定的数据
    void remove(T &data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _list.remove(data);
    }
};

class matcher
{
private:
    match_queue<int> _q_bronze;
    match_queue<int> _q_silver;
    match_queue<int> _q_gold;
    std::thread _th_bronze;
    std::thread _th_silver;
    std::thread _th_gold;
    room_manager *_rm;
    user_table *_ut;
    online_manager *_om;

private:
    void handle_match(match_queue<int> &mq)
    {
        while (1)
        {
            while (mq.size() < 2)
            {
                mq.wait();
            }
            //队列中玩家人数>=2会走到这里
            int uid1, uid2;
            bool ret = mq.pop(uid1);
            if (ret == false)
            {
                continue;
            }
            ret = mq.pop(uid2);
            if (ret == false)
            {
                this->add(uid1);
                continue;
            }
            //如果有一个玩家掉线，要把另一个玩家重新添加回队列
            wsserver_t::connection_ptr conn1 = _om->get_conn_from_hall(uid1);
            if (conn1.get() == nullptr)
            {
                this->add(uid2);
                continue;
            }
            wsserver_t::connection_ptr conn2 = _om->get_conn_from_hall(uid2);
            if (conn2.get() == nullptr)
            {
                this->add(uid1);
                continue;
            }
            //为两个玩家创建房间
            room_ptr rp = _rm->create_room(uid1, uid2);
            if (rp.get() == nullptr)
            {
                this->add(uid1);
                this->add(uid2);
                continue;
            }
            //给玩家发送匹配成功的消息
            Json::Value response;
            response["optype"] = "match_success";
            response["result"] = true;
            std::string body;
            json_util::Serialize(response, &body);
            conn1->send(body);
            conn2->send(body);
        }
    }
    void th_bronze_enter()
    {
        handle_match(_q_bronze);
    }
    void th_silver_enter()
    {
        handle_match(_q_silver);
    }
    void th_gold_enter()
    {
        handle_match(_q_gold);
    }

public:
    matcher(room_manager *rm, user_table *ut, online_manager *om) : _rm(rm), _ut(ut), _om(om),
                                                                    _th_bronze(std::thread(&matcher::th_bronze_enter, this)),
                                                                    _th_silver(std::thread(&matcher::th_silver_enter, this)),
                                                                    _th_gold(std::thread(&matcher::th_gold_enter, this))
    {
        DLOG("游戏匹配模块初始化完毕...");
    }
    bool add(int uid)
    {
        Json::Value user;
        bool ret = _ut->select_by_id(uid, user);
        if (ret == false)
        {
            DLOG("获取玩家:%d信息失败", uid);
            return false;
        }
        int score = user["score"].asInt();
        if (score < 2000)
        {
            _q_bronze.push(uid);
        }
        else if (score >= 3000)
        {
            _q_gold.push(uid);
        }
        else
        {
            _q_silver.push(uid);
        }
        return true;
    }
    bool del(int uid)
    {
        Json::Value user;
        bool ret = _ut->select_by_id(uid, user);
        if (ret == false)
        {
            DLOG("获取玩家:%d信息失败", uid);
            return false;
        }
        int score = user["score"].asInt();
        if (score < 2000)
        {
            _q_bronze.pop(uid);
        }
        else if (score >= 3000)
        {
            _q_gold.pop(uid);
        }
        else
        {
            _q_silver.pop(uid);
        }
        return true;
    }
};

#endif