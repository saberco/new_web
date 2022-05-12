#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "../webserver/webserver.h"

class Config{
public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char* argv[]);

    //端口号
    int PORT;
    //同步or异步
    int LOGWrite;
    //触发epoll模式
    int TRIGMode;
    //listen触发模式
    int LISTENTRIGMode;
    //connfd触发模式
    int CONNTRIGMode;
    //优雅关闭连接
    int OPT_LINGER;
    //数据库连接处数量
    int sql_num;
    //线程池内的线程数量
    int thread_num;
    //是否关闭日志
    int close_log;
    //并发模型选择
    int actor_model;
};







#endif