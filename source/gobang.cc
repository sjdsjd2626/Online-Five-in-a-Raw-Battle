
#include "logger.hpp"
#include "util.hpp"
#include"enterdb.hpp"
#include"manageOnlineGamer.hpp"
#define HOST "127.0.0.1"
#define PORT 3306
#define USER "root"
#define PASSWD "123456"
#define DBNAME "gobang"
void testmysql()
{
// LOG("%s:%d\n","hello",100);
    // ILOG("hello1");
    // DLOG("hello2");
    // ELOG("hello3");
    MYSQL *mysql = mysql_util::mysql_create(HOST, USER, PASSWD, DBNAME, PORT);
    std::string sql = "insert into stu values(null,'小黑',66,88,76);";
    int ret=mysql_util::mysql_exec(mysql, sql);
    if(ret==false)std::cout<<"main error"<<std::endl;
    mysql_util::mysql_destroy(mysql);
}
void testjson()
{
    Json::Value root;
    root["姓名"]="小明";
    root["年龄"]=18;
    root["成绩"].append(98);
    root["成绩"].append(56.5);
    root["成绩"].append(86.5);
    std::string result;
    json_util::Serialize(root,&result);
    std::cout<<result<<std::endl;
    Json::Value root1;
    json_util::UnSerialize(result,&root1);
    std::cout<<"姓名："<<root["姓名"].asString()<<std::endl;
    std::cout<<"年龄："<<root["年龄"].asInt()<<std::endl;
    int sz=root["成绩"].size();
    for(int i=0;i<sz;i++)
    {
        std::cout<<"成绩："<<root["成绩"][i].asFloat()<<std::endl;
    }
}
void teststring()
{
    std::string str=",,...,,123,,,456,789,,,";
    std::vector<std::string> ret;
    string_util::split(str,",",&ret);
    for(auto &e:ret)
    {
        std::cout<<e<<std::endl;
    }
}
void testfile()
{
    std::string body;
    file_util::read("./makefile",&body);
    std::cout<<body<<std::endl;
}
void testenterdb()
{
user_table ut(HOST,USER,PASSWD,DBNAME);
Json::Value val;
val["username"]="xiaohong";
val["password"]="123123";

// if(ut.login(val))
// {
//     DLOG("login success");
// }

// ut.insert(val);

// if(ut.select_by_name("xiaohei",val)){
// std::cout<<val["id"]<<std::endl;
// std::cout<<val["score"]<<std::endl;
// }

// if(ut.select_by_id(6,val)){
// std::cout<<val["id"]<<std::endl;
// std::cout<<val["score"]<<std::endl;
// }

// ut.win(3);
// ut.win(3);
// ut.win(3);

// ut.win(9);


// ut.lose(7);
// ut.lose(7);
// ut.lose(7);


}

void testonlineuser()
{
    online_manager om;
    wsserver_t::connection_ptr tmp;
    om.enter_game_hall(1,tmp);
    om.enter_game_room(2,tmp);
    if(om.is_in_game_hall(1))std::cout<<"1"<<std::endl;
    if(om.is_in_game_hall(2))std::cout<<"2"<<std::endl;
    if(om.is_in_game_room(1))std::cout<<"3"<<std::endl;
    if(om.is_in_game_room(2))std::cout<<"4"<<std::endl;

    om.get_conn_from_room(2);

    om.exit_game_hall(1);
    om.exit_game_room(2);
    if(om.is_in_game_hall(1))std::cout<<"11"<<std::endl;
    if(om.is_in_game_hall(2))std::cout<<"22"<<std::endl;
    if(om.is_in_game_room(1))std::cout<<"33"<<std::endl;
    if(om.is_in_game_room(2))std::cout<<"44"<<std::endl;

    
}
int main()
{
    testonlineuser();

    return 0;
}