#ifndef __HEAPTIMER_H__
#define __HEAPTIMER_H__

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <vector>
#include <unordered_map>

#include <time.h>
#include "../log/log.h"


//先声明保活定时器类
class util_timer;

//结构体保存客户端定时信息
//主要是address和socket文件描述符和定时信息
struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer{
public:
//id即文件描述符
    int id;
    time_t expire;
    client_data* user_data;     //指向用户数据
    //回调函数指针：从内核表删除事件
    void (* cb_func)(client_data *);
};

class Heap_timer{
public:
    Heap_timer(){m_heap.reserve(64);}
    ~Heap_timer(){clear_();}

    void add_timer(util_timer* timer);       //添加定时器，内部调用私有的add_timer,是一个接口
    void adjust_timer(util_timer* timer);    //调整定时器
    void del_timer(util_timer* timer);       //删除超时的
    void del_(size_t index);
    void tick();                             //脉搏函数,是定时任务处理函数，用于检查超时的连接


private:
    void pop();
    void siftup_(size_t index);
    void swapnode_(size_t index_i, size_t index_j);
    bool siftdown_(size_t index, size_t n);
    void clear_();
    std::vector<util_timer*> m_heap;
    //从文件描述符找到对应的index
    std::unordered_map<int, size_t> m_ref;
};


//设置定时器
class Utils{
public:
    Utils(){}
    ~Utils(){}

    void init(int timeslot);
    //设置文件描述符非阻塞，避免影响性能
    int setnonblocking(int fd);
    //内核事件注册为读事件，epoll et模式，开启epolloneshot
    //即将socketfd添加到epollfd监听中
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);
    //定时处理任务，不断发送SIGALRM信号
    void timer_handler();
    void show_error(int connfd, const char *info);

public:
    static int *u_pipedfd;
    Heap_timer m_heap_timer;
    static int u_epollfd;
    int m_TIMESLOT;

};

void cb_func(client_data* user_data);


#endif