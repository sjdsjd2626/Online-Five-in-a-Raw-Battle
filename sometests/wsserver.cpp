#include<iostream>
#include<string>
#include<functional>
#include<websocketpp/server.hpp>
#include<websocketpp/config/asio_no_tls.hpp>

typedef websocketpp::server<websocketpp::config::asio>wsserver_t;
void print(const char* str);
void wshttp_callback(wsserver_t*srv,websocketpp::connection_hdl hd1)//收到http请求调用这个函数
{
    wsserver_t::connection_ptr conn=srv->get_con_from_hdl(hd1);
    std::cout<<"body "<<conn->get_request_body()<<std::endl;
    websocketpp::http::parser::request req=conn->get_request();
    std::cout<<"method "<<req.get_method()<<std::endl;
    std::cout<<"uri "<<req.get_uri()<<std::endl;

    std::string body="<html><body><h1>Hello World</h1></body></html>";
    conn->set_body(body);
    conn->append_header("Content-Type","text/html");
    conn->set_status(websocketpp::http::status_code::ok);
    wsserver_t::timer_ptr tp=srv->set_timer(5000,std::bind(print,"helloworld"));//5000毫秒后执行print任务
    tp->cancel();//定时任务的取消实际上是取消定时，而立即执行任务
}
void print(const char* str){std::cout<<str<<std::endl;}

void wsopen_callback(wsserver_t*srv,websocketpp::connection_hdl hd1)//连接建立调用这个函数
{
    std::cout<<"websocket握手成功!!"<<std::endl;
}
void wsclose_callback(wsserver_t*srv,websocketpp::connection_hdl hd1)//连接断开调用这个函数
{
    std::cout<<"websocket连接断开!!"<<std::endl;
}
void wsmsg_callback(wsserver_t*srv,websocketpp::connection_hdl hd1,wsserver_t::message_ptr msg)//收到消息调用这个函数
{
    wsserver_t::connection_ptr conn=srv->get_con_from_hdl(hd1);
    std::cout<<"wsmsg: "<<msg->get_payload()<<std::endl;
    std::string rsp ="client say: "+msg->get_payload();
    conn->send(rsp,websocketpp::frame::opcode::text);
}
int main()
{
    //1.实例化server对象
    wsserver_t wssrv;
    //2.设置日志等级
    wssrv.set_access_channels(websocketpp::log::alevel::none);
    //3.初始化asio调度器
    wssrv.init_asio();
    wssrv.set_reuse_addr(true);
    //4.设置回调函数
    wssrv.set_http_handler(std::bind(wshttp_callback,&wssrv,std::placeholders::_1));//从左到右是形参的顺序，1，2是实参
    wssrv.set_open_handler(std::bind(wsopen_callback,&wssrv,std::placeholders::_1));
    wssrv.set_close_handler(std::bind(wsclose_callback,&wssrv,std::placeholders::_1));
    wssrv.set_message_handler(std::bind(wsmsg_callback,&wssrv,std::placeholders::_1,std::placeholders::_2));
    //5.设置监听端口
    wssrv.listen(5566);
    //6.开始获取新连接
    wssrv.start_accept();
    //7.启动服务器
    wssrv.run();
    return 0;
}