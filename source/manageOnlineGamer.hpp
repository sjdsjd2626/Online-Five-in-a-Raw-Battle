#ifndef _MANAGE_ONLINE_GAMER_
#define _MANAGE_ONLINE_GAMER_
#include <mutex>
#include <unordered_map>
#include "util.hpp"

class online_manager
{
private:
    std::mutex _mutex;
    std::unordered_map<int, wsserver_t::connection_ptr> _hall_user;
    std::unordered_map<int, wsserver_t::connection_ptr> _room_user;

public:
void enter_game_hall(int uid,wsserver_t::connection_ptr&conn)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _hall_user[uid]=conn;
}
void enter_game_room(int uid,wsserver_t::connection_ptr&conn)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _room_user[uid]=conn;
}
void exit_game_hall(int uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _hall_user.erase(uid);
}
void exit_game_room(int uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    _room_user.erase(uid);
}
bool is_in_game_hall(int uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto it=_hall_user.find(uid);
    if(it==_hall_user.end())
    {
        return false;
    }
    return true;
}
bool is_in_game_room(int uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto it=_room_user.find(uid);
    if(it==_room_user.end())
    {
        return false;
    }
    return true;
}
wsserver_t::connection_ptr get_conn_from_hall(int uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto it=_hall_user.find(uid);
    if(it==_hall_user.end())
    {
        return wsserver_t::connection_ptr();
    }
    return it->second;
}
wsserver_t::connection_ptr get_conn_from_room(int uid)
{
    std::unique_lock<std::mutex> lock(_mutex);
    auto it=_room_user.find(uid);
    if(it==_room_user.end())
    {
        return wsserver_t::connection_ptr();
    }
    return it->second;
}
};

#endif