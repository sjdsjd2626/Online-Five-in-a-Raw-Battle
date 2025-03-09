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
    void http_callback1(websocketpp::connection_hdl hd1)
    {
        wsserver_t::connection_ptr conn = _wssrv.get_con_from_hdl(hd1);
        websocketpp::http::parser::request req = conn->get_request();
        std::string method = req.get_method();
        std::string uri = req.get_uri();
        std::string pathname;
        if (uri == "/")
            pathname = _web_root + "/register.html";
        else
            pathname = _web_root + uri;
        std::string body;
        file_util::read(pathname, &body);
        conn->set_status(websocketpp::http::status_code::ok);
        conn->set_body(body);
    }
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
        return;
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
        websocketpp::http::parser::request req = conn->get_request();
        std::string req_body = conn->get_request_body();
        Json::Value login_info;
        Json::Value resq_json;
        bool ret = json_util::UnSerialize(req_body, &login_info);
        if (ret == false)
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
    }
    void info(wsserver_t::connection_ptr &conn)
    {
        // 用户信息获取功能请求的处理
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
    void wsopen_callback(websocketpp::connection_hdl hd1) // 连接建立调用这个函数
    {
    }
    void wsclose_callback(websocketpp::connection_hdl hd1) // 连接断开调用这个函数
    {
    }
    void wsmsg_callback(websocketpp::connection_hdl hd1, wsserver_t::message_ptr msg) // 收到消息调用这个函数
    {
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