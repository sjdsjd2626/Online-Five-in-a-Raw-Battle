#ifndef _M_ROOM_H_
#define _M_ROOM_H_
#include "logger.hpp"
#include "manageOnlineGamer.hpp"
#include "util.hpp"
#include "enterdb.hpp"

#define BOARD_ROW 15
#define BOARD_COL 15
#define WHITE_FLAG 1
#define BLACK_FLAG 2

typedef enum
{
    game_start,
    game_over
} room_status;
class room
{
private:
    int _room_id;
    room_status _room_status;
    int _player_count;
    int _white_id;
    int _black_id;
    user_table *_enterdb;
    online_manager *_online_user;
    std::vector<std::vector<int>> _board;

private:
    void add_forbidden_words(std::vector<std::string> &forbidden_words)
    {
        forbidden_words.push_back("傻逼");
        //......
    }
    // cross：横  vertival：竖
    int testcount(int row, int col, int cross, int vertival) // 看一排，看一列，看斜方向成不成立五子连珠
    {
        int count = 1;
        int flag = _board[row][col];
        int tmprow = row + cross;
        int tmpcol = col + vertival;
        while (tmprow >= 0 && tmprow < BOARD_ROW && tmpcol >= 0 && tmpcol < BOARD_COL && _board[tmprow][tmpcol] == flag)
        {
            count++;
            tmprow += cross;
            tmpcol += vertival;
        }
        tmprow = row - cross;
        tmpcol = col - vertival;
        while (tmprow >= 0 && tmprow < BOARD_ROW && tmpcol >= 0 && tmpcol < BOARD_COL && _board[tmprow][tmpcol] == flag)
        {
            count++;
            tmprow -= cross;
            tmpcol -= vertival;
        }
        return count;
    }
    int checkwin(int chess_row, int chess_col, int chess_uid)
    {
        if (testcount(chess_row, chess_col, 0, 1) >= 5 ||
            testcount(chess_row, chess_col, 1, 0) >= 5 ||
            testcount(chess_row, chess_col, 1, -1) >= 5 ||
            testcount(chess_row, chess_col, 1, 1) >= 5)
        {
            return chess_uid;
        }
        else
            return 0;
    }

public:
    room(int room_id, user_table *enterdb, online_manager *online_user) : _room_id(room_id), _room_status(game_start), _player_count(0), _enterdb(enterdb),
                                                                          _online_user(online_user), _board(BOARD_ROW, std::vector<int>(BOARD_COL, 0))
    {
        DLOG("%d room create success!!!", room_id);
    }
    ~room()
    {
        DLOG("%d room destroy success!!!", _room_id);
    }
    int GetWhiteUser()
    {
        return _white_id;
    }
    int GetBlackUser()
    {
        return _black_id;
    }
    int GetRoomId()
    {
        return _room_id;
    }
    int GetPlayerCount()
    {
        return _player_count;
    }
    void add_white_user(int uid)
    {
        _white_id=uid;
    }
    void add_black_user(int uid)
    {
        _black_id=uid;
    }
    // 处理下棋动作
    Json::Value handle_chess(Json::Value &req)
    {
        int chess_row = req["row"].asInt();
        int chess_col = req["col"].asInt();
        int chess_uid = req["uid"].asInt();

        Json::Value result;
        // 判断房间中两个玩家是否都在线，任意一个不在线，则是另一方胜利
        if (_online_user->is_in_game_room(_white_id) == false || _online_user->is_in_game_room(_black_id) == false)
        {
            result["optype"] = "put_chess";
            result["result"] = true;
            result["reason"] = "对方掉线，不战而胜";
            result["room_id"] = _room_id;
            result["uid"] = chess_uid;
            result["row"] = chess_row;
            result["col"] = chess_col;
            if (_online_user->is_in_game_room(_white_id) == false)
                result["winner"] = _black_id;
            else
                result["winner"] = _white_id;
            return result;
        }
        // 看这个位置有没有被占用
        if (_board[chess_row][chess_col] != 0)
        {
            result["optype"] = "put_chess";
            result["result"] = false;
            result["reason"] = "这个位置已经有了其他棋子";
            return result;
        }
        _board[chess_row][chess_col] = (chess_uid == _white_id ? WHITE_FLAG : BLACK_FLAG);
        // 判断是否有玩家胜利，如果有，返回玩家id，如果没有，返回0
        int winner_zero = checkwin(chess_row, chess_col, chess_uid);
        if (winner_zero == 0)
            result["reason"] = "目前没有人胜利";
        else
            result["reason"] = "五星连珠，战无敌";

        result["optype"] = "put_chess";
        result["result"] = true;
        result["room_id"] = _room_id;
        result["uid"] = chess_uid;
        result["row"] = chess_row;
        result["col"] = chess_col;
        result["winner"] = winner_zero;
        return result;
    }
    // 处理聊天动作
    Json::Value handle_chat(Json::Value &req)
    {
        Json::Value result;
        std::string message = req["massage"].asCString();
        std::vector<std::string> forbidden_words;
        add_forbidden_words(forbidden_words);
        for (auto &e : forbidden_words)
        {
            if (message.find(e) != std::string::npos) // 找到违禁词
            {
                result["optype"] = "chat";
                result["result"] = false;
                result["reason"] = "语言有违禁词，禁止发送";
                return result;
            }
        }
        result["optype"] = "chat";
        result["result"] = true;
        result["room_id"] = _room_id;
        result["uid"] = req["uid"].asInt();
        result["message"] = message.c_str();
        return result;
    }
    // 总的处理函数，根据请求类型，分别调用上面两个函数
    void handle_request(Json::Value &req)
    {
        // 判断请求的房间号是否与当前房间的房间号匹配
        int room_id = req["room_id"].asInt();
        if (room_id != _room_id)
        {
            ELOG("请求的房间号:%d 与当前房间的房间号:%d 不匹配，任务不执行", room_id, _room_id); // 这个明显不是前端的错误，不需要给前端返回
            return;
        }
        // 判断请求类型，分别交给不同函数
        Json::Value result;
        std::string type = req["optype"].asCString();
        if (type == "put_chess")
        {
            result = handle_chess(req);
            if (result["winner"].asInt() != 0) // 如果有胜利者的话游戏就结束了
            {
                int winner_id = result["winner"].asInt();
                int loser_id = (winner_id == _white_id ? _black_id : _white_id);
                _enterdb->win(winner_id); // 写入数据库操作
                _enterdb->lose(loser_id);
                _room_status = game_over;
            }
        }
        else if (type == "chat")
        {
            result = handle_chat(req);
        }
        else
        {
            result["optype"] = req["optype"].asCString();
            result["result"] = false;
            result["reason"] = "未知请求类型";
            broadcast(result, req["uid"].asInt()); // 谁给我的未知请求，我给他返回即可，不用给房间内的每个人都返回
            ELOG("未知请求，无法处理");
            return;
        }
        broadcast(result);
    }
    // 处理玩家退出房间动作
    void handle_exit(int uid)
    {
        Json::Value result;
        // 如果是下棋中退出，则对方胜利；如果下棋结束退出，则是正常退出
        if (_room_status == game_start)
        {
            result["optype"] = "put_chess";
            result["result"] = true;
            result["reason"] = "对方掉线，不战而胜";
            result["room_id"] = _room_id;
            result["uid"] = uid;
            result["row"] = -1;
            result["col"] = -1;
            result["winner"] = (uid == _white_id ? _black_id : _white_id);
        }
        broadcast(result);
        _player_count--;
    }
    // 将指定的信息广播给房间中的所有玩家
    void broadcast(Json::Value &rsp, int sign_id = 0) // 0默认发送给所有玩家，非0发送给指定玩家
    {
        std::string body;
        json_util::Serialize(rsp, &body);
        // 获取房间中所有用户的通信连接，发送响应信息
        if (sign_id == 0)
        {
            wsserver_t::connection_ptr wconn = _online_user->get_conn_from_room(_white_id);
            if (wconn.get() != nullptr)
            {
                wconn->send(body);
            }
            wsserver_t::connection_ptr bconn = _online_user->get_conn_from_room(_black_id);
            if (bconn.get() != nullptr)
            {
                bconn->send(body);
            }
        }
        else
        {
            wsserver_t::connection_ptr conn = _online_user->get_conn_from_room(sign_id);
            if (conn.get() != nullptr)
            {
                conn->send(body);
            }
        }
    }
};

using room_ptr = std::shared_ptr<room>;
class room_manager
{
private:
    int _next_rid;
    std::mutex _mutex;
    user_table *_enterdb;
    online_manager *_online_user;
    std::unordered_map<int, room_ptr> _rooms;
    std::unordered_map<int, int> _user_to_room;

public:
    room_manager(user_table *enterdb, online_manager *online_user):
    _next_rid(1),_enterdb(enterdb),_online_user(online_user)
    {
        DLOG("房间管理模块初始化完毕");
    }
    ~room_manager()
    {
        DLOG("房间管理模块即将销毁");
    }
    // 为两个用户创建房间，并返回房间的智能指针管理对象
    room_ptr create_room(int uid1, int uid2)
    {
        //判断两个用户是否都还在游戏大厅中
        if(_online_user->is_in_game_hall(uid1)==false)
        {
            DLOG("用户%d不在游戏大厅，创建房间失败",uid1);
            return room_ptr();
        }
        if(_online_user->is_in_game_hall(uid2)==false)
        {
            DLOG("用户%d不在游戏大厅，创建房间失败",uid2);
            return room_ptr();
        }
        //创建房间，将用户信息添加到房间中
        std::unique_lock<std::mutex> lock(_mutex);//首先要保护_next_rid，并且要保护map（插入数据和查要隔离）
        room_ptr rp(new room(_next_rid,_enterdb,_online_user));
        rp->add_white_user(uid1);
        rp->add_black_user(uid2);
        //将房间管理起来
        _rooms.insert(std::make_pair(_next_rid,rp));
        _user_to_room.insert(std::make_pair(uid1,_next_rid));
        _user_to_room.insert(std::make_pair(uid2,_next_rid));
        _next_rid++;
        return rp;
    }
    // 通过房间ID获取房间信息
    room_ptr get_room_by_rid(int rid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it=_rooms.find(rid);
        if(it==_rooms.end())
        {
            return room_ptr();
        }
        return it->second;
    }
    // 通过用户ID获取房间信息
    room_ptr get_room_by_uid(int uid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        //通过用户ID获得房间ID
        auto uit=_user_to_room.find(uid);
        if(uit==_user_to_room.end())
        {
            return room_ptr();
        }
        //通过房间ID获得房间信息
        auto rit=_rooms.find(uit->second);
        if(rit==_rooms.end())
        {
            return room_ptr();
        }
        return rit->second;
    }
    // 通过房间ID销毁房间
    void remove_room(int rid)
    {
        room_ptr rp=get_room_by_rid(rid);
        if(rp.get()==nullptr)
        {
            return;
        }
        int uid1=rp->GetWhiteUser();
        int uid2=rp->GetBlackUser();
        std::unique_lock<std::mutex> lock(_mutex);
        _user_to_room.erase(uid1);
        _user_to_room.erase(uid2);
        _rooms.erase(rid);
    }
    // 删除房间中指定用户，如果房间中没有用户了，则销毁房间，用户连接断开时调用
    void remove_user_of_room(int uid)
    {
        room_ptr rp=get_room_by_uid(uid);
        if(rp.get()==nullptr)
        {
            return;
        }
        rp->handle_exit(uid);
        if(rp->GetPlayerCount()==0)
        {
            remove_room(rp->GetRoomId());
        }
    }
};

#endif