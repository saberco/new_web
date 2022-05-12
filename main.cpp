#include"./Config/Config.h"

using namespace std;
int main(int argc, char* argv[]){

    string user = "root";
    string passwd = "123456";
    string databasename = "test";
    int m_close_log = 0;
    Config config;
    config.parse_arg(argc, argv);


    Webserver server;

    //初始化

    server.init(config.PORT, user, passwd, databasename ,config.LOGWrite, config.OPT_LINGER, config.TRIGMode, config.sql_num, config.thread_num,
            config.close_log, config.actor_model);

    //初始化日志

    server.log_write();

    //数据库

    server.sql_pool();

    //创建线程池

    server.thread_pool();

    //初始化epoll

    server.trig_mode();
    server.eventListen();

    //运行
    LOG_INFO("%s", "start Listen");
    server.eventLoop();
    return 0;
}