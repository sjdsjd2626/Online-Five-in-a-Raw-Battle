#include<iostream>
#include<string>
#include<sstream>
#include<jsoncpp/json/json.h>

std::string Serialize()
{
    //1.将需要进行序列化的数据，存储在Json::Value对象中
    Json::Value root;
    root["姓名"]="小明";
    root["年龄"]=18;
    root["成绩"].append(98);
    root["成绩"].append(56.5);
    root["成绩"].append(86.5);
    //2.实例化一个工厂类对象
    Json::StreamWriterBuilder swb;
    //3.使用工厂类对象生产一个对象
    Json::StreamWriter* sw=swb.newStreamWriter();
    //4.使用StreamWriter，对Josn::Value中存储的数据进行序列化
    std::stringstream ss;
    int ret=sw->write(root,&ss);
    if(ret!=0)
    {
        std::cout<<"Serialize failed!"<<std::endl;
        return "";
    }
    std::cout<<ss.str()<<std::endl;
    delete sw;
    return ss.str();

}
void UnSerialize(const std::string& str)
{
    //1.实例化一个工厂类对象
    Json::CharReaderBuilder crb;
    //2.使用工厂类对象生产一个对象
    Json::CharReader*cr=crb.newCharReader();
    //3.定义一个Json::Value对象用于存储解析后的数据
    Json::Value root;
    std::string err;//用于保存错误信息
    //4.使用CharReader对象进行json格式字符串的反序列化
    //parse(char*start,char*end,Json::Value *val,string*err);
    bool ret=cr->parse(str.c_str(),str.c_str()+str.size(),&root,&err);
    if(ret==false)
    {
        std::cout<<"UnSerialize failed!"<<std::endl;
        return ;
    }
    //5.逐个去访问root中的数据
    std::cout<<"姓名："<<root["姓名"].asString()<<std::endl;
    std::cout<<"年龄："<<root["年龄"].asInt()<<std::endl;
    int sz=root["成绩"].size();
    for(int i=0;i<sz;i++)
    {
        std::cout<<"成绩："<<root["成绩"][i].asFloat()<<std::endl;
    }
    delete cr;
} 
int main()
{
    std::string ret=Serialize();
    UnSerialize(ret);
    return 0;
}