#ifndef _M_SESSION_H
#define _M_SESSION_H
#include <unordered_map>
#include "util.hpp"
typedef enum
{
    UNLOGIN,
    LOGIN
} ss_statu;
class session
{
private:
    int _ssid;
    int _uid;
    ss_statu _statu;           // 未登录、已经登录
    wsserver_t::timer_ptr _tp; // session关联的定时器
public:
    session(int ssid) : _ssid(ssid) { DLOG("session %p 被创建", this); }
    ~session() { DLOG("session %p 被释放", this); }
    void set_user(int uid) { _uid = uid; }
    int get_user() { return _uid; }
    int ssid(){return _ssid;}
    bool is_login() { return (_statu == LOGIN); }
    void set_statu(ss_statu statu){_statu=statu;}
    void set_timer_of_session(const wsserver_t::timer_ptr &tp) { _tp = tp; }
    wsserver_t::timer_ptr &get_timer_of_session() { return _tp; }
};

#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1
using session_ptr=std::shared_ptr<session>;
class session_maneger
{
private:
    int _next_ssid;
    std::mutex _mutex;
    std::unordered_map<int,session_ptr>_sessions;
    wsserver_t*_server;
public:
    session_maneger(wsserver_t*srv):_next_ssid(1),_server(srv)
    {
        DLOG("session 管理器初始化完毕");
    }
    ~session_maneger()
    {
        DLOG("session 管理器即将销毁");
    }
    session_ptr create_session(int uid,ss_statu statu)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        session_ptr ssp(new session(_next_ssid));
        ssp->set_statu(statu);
        _sessions.insert(std::make_pair(_next_ssid,ssp));
        _next_ssid++;
        return ssp;
    }
    session_ptr get_session_by_ssid(int ssid)
    {
        std::unique_lock<std::mutex>lock(_mutex);
        auto it=_sessions.find(ssid);
        if(it==_sessions.end())return session_ptr();
        else return it->second;
    }
    void append_session(const session_ptr&ssp)
    {
        std::unique_lock<std::mutex>lock(_mutex);
        _sessions.insert(std::make_pair(ssp->ssid(),ssp));
    }
    void remove_session(int ssid)
    {
        std::unique_lock<std::mutex>lock(_mutex);
        _sessions.erase(ssid);
    }
    void set_session_expire_time(int ssid,int ms)
    {
        session_ptr ssp=get_session_by_ssid(ssid);
        if(ssp.get()==nullptr)return;//根本就没有这个会话
        wsserver_t::timer_ptr tp=ssp->get_timer_of_session();//tp就是定时任务
        if(tp.get()==nullptr&&ms==SESSION_FOREVER)return;//没有定时任务（定时任务就是删除会话）并且不想删除会话
        else if(tp.get()==nullptr&&ms!=SESSION_FOREVER)//没有删除会话的定时任务，但是想删除会话,那就去设置定时任务
        {
            wsserver_t::timer_ptr tmp_tp=_server->set_timer(ms,std::bind(&session_maneger::remove_session,this,ssid));
            ssp->set_timer_of_session(tmp_tp);
        }
        else if(tp.get()!=nullptr&&ms==SESSION_FOREVER)//有删除会话的定时任务，但是不想删除会话,那就去删除定时任务
        {
            tp->cancel();//立即删除会话，所以后面要再次插入会话
            ssp->set_timer_of_session(wsserver_t::timer_ptr());
            _server->set_timer(0,std::bind(&session_maneger::append_session,this,ssp));
        }
        else if(tp.get()!=nullptr&&ms!=SESSION_FOREVER)//这个就需要重置删除时间
        {
            tp->cancel();//取消
            _server->set_timer(0,std::bind(&session_maneger::append_session,this,ssp));//添加
            ssp->set_timer_of_session(wsserver_t::timer_ptr());
            wsserver_t::timer_ptr tmp_tp=_server->set_timer(ms,std::bind(&session_maneger::remove_session,this,ssid));
            ssp->set_timer_of_session(tmp_tp);
        }
    }

};


#endif