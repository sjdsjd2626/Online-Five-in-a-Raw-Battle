#ifndef _M_SERVER_H
#define _M_SERVER_H
#include "util.hpp"
#include "enterdb.hpp"
#include "manageOnlineGamer.hpp"
#include "room.hpp"
#include "matcher.hpp"
#include "session.hpp"

#define WWWROOT "./wwwroot"
class gobang_server
{
private:
    std::string _web_root;
    wsserver_t _wssrv;
    user_table _ut;
    online_manager _om;
    room_manager _rm;
    session_maneger _sm;
    matcher _mm;

private:
    // void http_callback1(websocketpp::connection_hdl hd1)
    // {
    //     wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hd1);
    //     websocketpp::http::parser::request req = conn->get_request();
    //     std::string method = req.get_method();
    //     std::string uri = req.get_uri();
    //     std::string pathname;
    //     if (uri == "/")
    //         pathname = _web_root + "/register.html";
    //     else
    //         pathname = _web_root + uri;
    //     std::string body;
    //     file_util::read(pathname, &body);
    //     conn->set_status(websocketpp::http::status_code::ok);
    //     conn->set_body(body);
    // }
    void file_handler(wsserver_t::connection_ptr &conn)
    {
        // 静态资源请求的处理
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        std::string realpath = _web_root + uri;
        if (realpath.back() == '/') // 如果是个目录,就是ip:port/
        {
            realpath += "login.html";
        }
        Json::Value resp_json;
        std::string body;
        bool ret = file_util::read(realpath, &body);
        if (ret == false)
        {
            file_util::read(_web_root + "/notfound.html", &body);
            conn->set_body(body);
            conn->set_status(websocketpp::http::status_code::not_found);
            return;
        }
        conn->set_body(body);
        conn->set_status(websocketpp::http::status_code::ok);
    }
    void http_resp(wsserver_t::connection_ptr &conn, bool result, websocketpp::http::status_code::value code, const std::string &reason)
    {
        Json::Value resp_json;
        resp_json["result"] = result;
        resp_json["reason"] = reason;
        std::string resp_body;
        json_util::Serialize(resp_json, &resp_body);
        conn->set_status(code);
        conn->set_body(resp_body);
        conn->append_header("Content-type", "application/json");
    }
    void reg(wsserver_t::connection_ptr &conn)
    {
        // 用户注册功能请求的处理
        std::string req_body = conn->get_request_body();
        Json::Value login_info;
        bool ret = json_util::UnSerialize(req_body, &login_info);
        if (ret == false) // 反序列化失败，不是json格式
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请求的正文格式错误");
        if (login_info["username"].isNull() || login_info["password"].isNull())
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请输入用户名/密码");
        ret = _ut.insert(login_info);
        if (ret == false)
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "用户名已经被占用");
        return http_resp(conn, true, websocketpp::http::status_code::ok, "注册用户成功");
    }
    void login(wsserver_t::connection_ptr &conn)
    {
        // 用户登录功能请求的处理
        std::string req_body = conn->get_request_body();
        Json::Value login_info;
        bool ret = json_util::UnSerialize(req_body, &login_info);
        if (ret == false) // 反序列化失败，不是json格式
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请求的正文格式错误");
        if (login_info["username"].isNull() || login_info["password"].isNull())
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "请输入用户名/密码");
        ret = _ut.login(login_info);
        if (ret == false)
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "用户名/密码错误");
        // 如果登录成功，给客户端创建session，并通过cookie返回给客户端
        int uid = login_info["id"].asInt();
        session_ptr ssp = _sm.create_session(uid, LOGIN);
        if (ssp.get() == nullptr)
        {
            DLOG("创建会话失败");
            return http_resp(conn, false, websocketpp::http::status_code::internal_server_error, "创建会话失败");
        }
        // 设置session过期时间
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
        std::string cookie_str = "SSID=" + std::to_string(ssp->ssid());
        conn->append_header("Set-Cookie", cookie_str);
        return http_resp(conn, true, websocketpp::http::status_code::ok, "登录成功");
    }
    bool get_cookie_val(const std::string &cookie_ptr, const std::string &key, std::string &val)
    {
        // cookie中的信息以“; ”为分隔，例如“SSID=xxx; path=xxx”
        std::string sep = "; ";
        std::vector<std::string> cookie_arr;
        string_util::split(cookie_ptr, sep, &cookie_arr);
        for (auto e : cookie_arr)
        {
            // e的格式就是“SSID=xxx”，当然可能有多个=，这样按不合规处理
            std::vector<std::string> tmp;
            string_util::split(e, "=", &tmp);
            if (tmp.size() != 2)
            {
                continue;
            }
            if (tmp[0] == key)
            {
                val = tmp[1];
                return true;
            }
        }
        return false;
    }
    void info(wsserver_t::connection_ptr &conn)
    {
        // 用户信息获取功能请求的处理
        std::string cookie_str = conn->get_request_header("Cookie");
        if (cookie_str.empty())
        {
            DLOG("找不到cookie信息");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "找不到cookie信息");
        }
        std::string ssid_str;
        bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
        if (ret == false)
        {
            DLOG("找不到ssid信息");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "找不到ssid信息");
        }
        // 找到了ssid，就要去找会话信息
        session_ptr ssp = _sm.get_session_by_ssid(std::stoi(ssid_str));
        if (ssp.get() == nullptr)
        {
            DLOG("登陆过期，请重新登陆");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "登陆过期，请重新登陆");
        }
        int uid = ssp->get_user();
        Json::Value user_info;
        ret = _ut.select_by_id(uid, user_info);
        if (ret == false)
        {
            DLOG("找不到用户信息");
            return http_resp(conn, false, websocketpp::http::status_code::bad_request, "找不到用户信息");
        }
        std::string body;
        json_util::Serialize(user_info, &body);
        conn->set_body(body);
        conn->append_header("Content-type", "application/json");
        conn->set_status(websocketpp::http::status_code::ok);
        // 刷新session的过期时间
        DLOG("%s 的session过期时间刷新", user_info["username"].asCString());
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
    }
    void http_callback(websocketpp::connection_hdl hd1) // 收到http请求调用这个函数
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hd1);
        websocketpp::http::parser::request req = conn->get_request();
        std::string method = req.get_method();
        std::string uri = req.get_uri();
        // DLOG("%s and %s",method.c_str(),uri.c_str());
        if (method == "POST" && uri == "/reg")
        {
            return reg(conn);
        }
        else if (method == "POST" && uri == "/login")
        {
            return login(conn);
        }
        else if (method == "GET" && uri == "/info")
        {
            return info(conn);
        }
        else
        {
            return file_handler(conn);
        }
    }
    session_ptr get_session_by_cookie(wsserver_t::connection_ptr conn) // 验证用户的session信息
    {
        Json::Value err_resp;
        std::string cookie_str = conn->get_request_header("Cookie");
        if (cookie_str.empty())
        {
            DLOG("找不到cookie信息");
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "没有找到cookie信息";
            err_resp["result"] = false;
            std::string body;
            json_util::Serialize(err_resp, &body);
            conn->send(body);
            return session_ptr();
        }
        std::string ssid_str;
        bool ret = get_cookie_val(cookie_str, "SSID", ssid_str);
        if (ret == false)
        {
            DLOG("找不到ssid信息");
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "找不到ssid信息";
            err_resp["result"] = false;
            std::string body;
            json_util::Serialize(err_resp, &body);
            conn->send(body);
            return session_ptr();
        }
        session_ptr ssp = _sm.get_session_by_ssid(std::stoi(ssid_str));
        if (ssp.get() == nullptr)
        {
            DLOG("登陆过期，请重新登陆");
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "登陆过期，请重新登陆";
            err_resp["result"] = false;
            std::string body;
            json_util::Serialize(err_resp, &body);
            conn->send(body);
            return session_ptr();
        }
        return ssp;
    }
    void wsopen_game_hall(wsserver_t::connection_ptr conn)
    {
        // 查看有没有session信息
        session_ptr ssp = get_session_by_cookie(conn);
        // 判断玩家是否重复登陆
        Json::Value err_resp;
        if (_om.is_in_game_hall(ssp->get_user()) || _om.is_in_game_room(ssp->get_user()))
        {
            DLOG("玩家重复登录");
            err_resp["optype"] = "hall_ready";
            err_resp["reason"] = "玩家重复登录";
            err_resp["result"] = false;
            std::string body;
            json_util::Serialize(err_resp, &body);
            conn->send(body);
            return;
        }
        // 将用户添加到游戏大厅
        _om.enter_game_hall(ssp->get_user(), conn);
        // 给客户端响应成功信息
        Json::Value resp_json;
        resp_json["optype"] = "hall_ready";
        resp_json["result"] = true;
        std::string body;
        json_util::Serialize(resp_json, &body);
        conn->send(body);
        // 将session时常设为永久存在
        DLOG("%d 号session为永久存在", ssp->ssid());
        _sm.set_session_expire_time(ssp->ssid(), SESSION_FOREVER);
    }
    void wsopen_game_room(wsserver_t::connection_ptr conn)
    {
        // 查看有没有session信息
        session_ptr ssp = get_session_by_cookie(conn);
        // 判断玩家是否重复登陆
        Json::Value err_resp;
        if (_om.is_in_game_hall(ssp->get_user()) || _om.is_in_game_room(ssp->get_user()))
        {
            DLOG("玩家重复登录");
            err_resp["optype"] = "room_ready";
            err_resp["reason"] = "玩家重复登录";
            err_resp["result"] = false;
            std::string body;
            json_util::Serialize(err_resp, &body);
            conn->send(body);
            return;
        }
        room_ptr rp = _rm.get_room_by_uid(ssp->get_user());
        if (rp.get() == nullptr)
        {
            err_resp["optype"] = "room_ready";
            err_resp["reason"] = "没有找到玩家的房间信息";
            err_resp["result"] = false;
            std::string body;
            json_util::Serialize(err_resp, &body);
            conn->send(body);
            return;
        }
        // 将用户添加到在线用户管理的房间中
        _om.enter_game_room(ssp->get_user(), conn);
        // 将session时间设置为永久
        _sm.set_session_expire_time(ssp->ssid(), SESSION_FOREVER);
        Json::Value resp;
        resp["optype"] = "room_ready";
        resp["result"] = true;
        resp["room_id"] = rp->GetRoomId();
        resp["uid"] = ssp->get_user();
        resp["white_id"] = rp->GetWhiteUser();
        resp["black_id"] = rp->GetBlackUser();
        std::string body;
        json_util::Serialize(resp, &body);
        conn->send(body);
    }
    void wsopen_callback(websocketpp::connection_hdl hd1) // 连接建立了调用这个函数
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hd1);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        { // 建立了游戏大厅的长连接
            return wsopen_game_hall(conn);
        }
        else if (uri == "/room")
        { // 建立了游戏房间的长连接
            return wsopen_game_room(conn);
        }
    }
    void wsclose_game_hall(wsserver_t::connection_ptr conn)
    {
        // 查看有没有session信息
        session_ptr ssp = get_session_by_cookie(conn);
        // 将玩家从游戏大厅中移除
        _om.exit_game_hall(ssp->get_user());
        // 将session重新设置为定时销毁
        DLOG("%d 号session重新设置为定时销毁", ssp->ssid());
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
    }
    void wsclose_game_room(wsserver_t::connection_ptr conn)
    {
        // 查看有没有session信息
        session_ptr ssp = get_session_by_cookie(conn);
        // 将玩家从游戏大厅中移除
        _om.exit_game_room(ssp->get_user());
        // 将session重新设置为定时销毁
        DLOG("%d 号session重新设置为定时销毁", ssp->ssid());
        _sm.set_session_expire_time(ssp->ssid(), SESSION_TIMEOUT);
        _rm.remove_user_of_room(ssp->get_user());
    }
    void wsclose_callback(websocketpp::connection_hdl hd1) // 连接断开调用这个函数
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hd1);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        { // 断开了游戏大厅的长连接
            return wsclose_game_hall(conn);
        }
        else if (uri == "/room")
        { // 断开了游戏房间的长连接
            return wsclose_game_room(conn);
        }
    }
    void wsmessage_game_hall(wsserver_t::connection_ptr conn, wsserver_t::message_ptr msg)
    {
        // 查看有没有session信息
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
            return;
        Json::Value resp_json;
        std::string resp_body;
        // 获取请求信息
        std::string req_body = msg->get_payload();
        Json::Value req_json;
        bool ret = json_util::UnSerialize(req_body, &req_json);
        if (ret == false)
        {
            resp_json["result"] = false;
            resp_json["reason"] = "请求信息解析失败";
            json_util::Serialize(resp_json, &resp_body);
            conn->send(resp_body);
            return;
        }
        // 用户被加入匹配队列
        if (!req_json["optype"].isNull() && req_json["optype"].asString() == "match_start")
        {
            DLOG("%d 号用户被加入匹配队列", ssp->get_user());
            _mm.add(ssp->get_user());
            resp_json["optype"] = "match_start";
            resp_json["result"] = true;
            json_util::Serialize(resp_json, &resp_body);
            conn->send(resp_body);
            return;
        }
        // 用户被移除匹配队列
        else if (!req_json["optype"].isNull() && req_json["optype"].asString() == "match_stop")
        {
            DLOG("%d 号用户被移除匹配队列", ssp->get_user());
            _mm.del(ssp->get_user());
            resp_json["optype"] = "match_stop";
            resp_json["result"] = true;
            json_util::Serialize(resp_json, &resp_body);
            conn->send(resp_body);
            return;
        }
        resp_json["optype"] = "unknown";
        resp_json["result"] = false;
        json_util::Serialize(resp_json, &resp_body);
        conn->send(resp_body);
    }
    void wsmessage_game_room(wsserver_t::connection_ptr conn, wsserver_t::message_ptr msg)
    {
        Json::Value resp_json;
        // 查看有没有session信息
        session_ptr ssp = get_session_by_cookie(conn);
        if (ssp.get() == nullptr)
            return;
        // 获取客户端房间信息
        room_ptr rp = _rm.get_room_by_uid(ssp->get_user());
        if (rp.get() == nullptr)
        {
            resp_json["optype"] = "unknown";
            resp_json["result"] = false;
            resp_json["reason"] = "没有找到玩家房间信息";
            std::string resp_body;
            json_util::Serialize(resp_json, &resp_body);
            conn->send(resp_body);
            return;
        }
        // 进行反序列化，看看是不是json格式
        Json::Value req_json;
        std::string req_body = msg->get_payload();
        bool ret = json_util::UnSerialize(req_body, &req_json);
        if (ret == false)
        {
            resp_json["optype"] = "unknown";
            resp_json["result"] = false;
            resp_json["reason"] = "请求解析失败";
            std::string resp_body;
            json_util::Serialize(resp_json, &resp_body);
            conn->send(resp_body);
            return;
        }
        // 通过房间管理模块进行消息请求的处理
        DLOG("通过房间管理模块进行消息请求的处理");
        return rp->handle_request(req_json);
    }
    void wsmsg_callback(websocketpp::connection_hdl hd1, wsserver_t::message_ptr msg) // 收到消息调用这个函数
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hd1);
        websocketpp::http::parser::request req = conn->get_request();
        std::string uri = req.get_uri();
        if (uri == "/hall")
        { // 游戏大厅收到消息
            return wsmessage_game_hall(conn, msg);
        }
        else if (uri == "/room")
        { // 游戏房间收到消息
            return wsmessage_game_room(conn, msg);
        }
    }




public:
    gobang_server(const std::string &host,
                  const std::string &username,
                  const std::string &password,
                  const std::string &dbname,
                  uint16_t port = 3306,
                  const std::string &wwwroot = WWWROOT) : _web_root(wwwroot), _ut(host, username, password, dbname, port), _rm(&_ut, &_om), _sm(&_wssrv), _mm(&_rm, &_ut, &_om)
    {
        _wssrv.set_access_channels(websocketpp::log::alevel::none);
        _wssrv.init_asio();
        _wssrv.set_reuse_addr(true);
        _wssrv.set_http_handler(std::bind(&gobang_server::http_callback, this, std::placeholders::_1)); // 从左到右是形参的顺序，1，2是实参
        _wssrv.set_open_handler(std::bind(&gobang_server::wsopen_callback, this, std::placeholders::_1));
        _wssrv.set_close_handler(std::bind(&gobang_server::wsclose_callback, this, std::placeholders::_1));
        _wssrv.set_message_handler(std::bind(&gobang_server::wsmsg_callback, this, std::placeholders::_1, std::placeholders::_2));
    }
    void start(int port)
    {
        _wssrv.listen(port);
        _wssrv.start_accept();
        _wssrv.run();
    }
};

#endif