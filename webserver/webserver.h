#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include"../http_conn/http_conn.h"
#include"../threadpool/threadpool.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class Webserver{
public:
    Webserver();
    ~Webserver();
    //进行初始化
    void init(int port, std::string user, std::string password, std::string databaseName, 
              int log_write, int opt_linger, int TRIGMode, int sql_num,
              int thread_num, int close_log, int actor_model);
    //创建线程池
    void thread_pool();
    //初始化数据库连接池
    void sql_pool();
    //初始化日志系统
    void log_write();
    //设置epoll触发模式
    void trig_mode();
    //开启epoll监听
    void eventListen();

    //初始化定时器
    void timer(int connfd, struct sockaddr_in client_address);
    //调整定时器
    void adjust_timer(util_timer * timer);
    //删除定时器
    void deal_timer(util_timer * timer, int sockfd);
    //http处理用户数据
    bool dealclientdata();
    //处理定时器信号
    bool dealwithsignal(bool& timeout, bool &stop_server);
    //处理客户连接上受到的数据
    void dealwithread(int sockfd);

    //写操作
    void dealwithwrite(int sockfd);
    //服务器主线程
    void eventLoop();

private:
    //基础成员
    int m_port;         //端口号
    char *m_root;      //资源路径
    int m_log_write;   //开启写日志
    int m_close_log;    //关闭日志
    int m_actormodel;   

    int m_pipefd[2];    //管道
    int m_epollfd;      //主线程不需要静态
    http_conn* users;   //用户

    //数据库相关
    connection_pool* m_connPool;    //连接池实例
    std::string m_user;             //登录数据库的用户名
    std::string m_password;         //登录数据库的密码
    std::string m_databaseName;     //使用的数据库名
    int m_sql_num;                  //数据库连接线程数量

    //线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;

    //epoll event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER;
    int m_TRIGMode;
    int m_LISTENTRIGMode;
    int m_CONNTRIGMode;

    //定时器相关
    client_data * users_timer;
    Utils utils;
};

#endif