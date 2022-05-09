
#include"timerlst.h"


sort_timer_lst::sort_timer_lst():head(nullptr), tail(nullptr){}

sort_timer_lst::~sort_timer_lst(){
    util_timer* cur = head;
    while(cur){
        head = cur->next;
        delete cur;
        cur = head;
    }
}
//添加定时器
void sort_timer_lst::add_timer(util_timer * timer){
    if(!timer)return;
    if(!head){
        head = tail = timer;
        return;
    }
    //定时器按照超时时间expire从小到大排序，找到合适的位置进行timer的插入,如果比当前头节点小则直接作为新头，如果不是则调用add_timer私有函数
    if(timer->expire < head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer,head);
}

//不考虑为头节点的情况，时间复杂度为O(n)
void sort_timer_lst::add_timer(util_timer* timer, util_timer* lst_head){
    util_timer* prev = lst_head;
    util_timer* cur = prev->next;
    while(cur){
        if(timer->expire < cur->expire){
            prev->next = timer;
            timer->next = cur;
            cur->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = cur;
        cur = cur->next;
    }

    //如果是遍历到尾结点
    if(!cur){
        prev->next = timer;
        timer->next = nullptr;
        timer->prev = prev;
        tail = timer;
    }
}

void sort_timer_lst::adjust_timer(util_timer * timer){
    //当任务发生变化时，调整定时器在链表中的位置，即不活跃变成活跃时，超时时间会变大，所以需要调整
    if(!timer)return;
    util_timer * cur = timer->next;

    //如果被调整的是在尾部或者还是比下一个超时值要小，不进行调整
    if(!cur || (timer->expire < cur->expire))return;

    //如果需要调整的是在链表头部，取出并重新插入
    if(timer == head){
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    }//如果是在链表内部，取出并重新插入
    else{
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

//删除定时器
void sort_timer_lst::del_timer(util_timer * timer){
    if(!timer)return;
    //如果链表中仅有一个定时器
    if((timer==head) && (timer==tail)){
        delete timer;
        head = nullptr;
        tail = nullptr;
        return;
    }
    //如果要删除的是头节点
    if(timer == head){
        head = timer->next;
        head -> prev = nullptr;
        delete timer;
        return;
    }

    //如果要删除的是尾结点
    if(timer == tail){
        tail = timer->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }

    //如果是内部节点
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
//脉搏函数即定时器处理函数,需要主程序定时调用
void sort_timer_lst::tick(){
    if(!head)return;
    //获取当前时刻和超时时刻做对比
    time_t cur = time(NULL);
    util_timer * tmp = head;
    //从头开始查询，找到第一个比超时时间小的
    while(tmp){
        //如果当前时间小于定时器的超时时间，那么以后也没有超时的，直接退出
        if(cur<tmp->expire)break;
        //大于则往下执行,回调函数执行定时事件，删除监听关闭文件描述符
        tmp->cb_func(tmp->user_data);
        //从升序链表删除并重置头节点
        head = tmp->next;
        if(head){
            head->prev = nullptr;
        }
        delete tmp;
        //继续循环
        tmp = head;
    }
}

//设置定时器
//设置超时时间值
void Utils::init(int timeslot){
    m_TIMESLOT = timeslot;
}

//设置文件描述符非阻塞,返回值是旧的文件描述符,EPOLL ET模式要设置文件描述符非阻塞，如果不是非阻塞，那么处理读或写事件会在最后一次阻塞
int Utils::setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//向epollfd中添加监听事件
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode){
    epoll_event event;
    event.data.fd = fd;
    if(1 == TRIGMode){
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else{
        event.events = EPOLLIN | EPOLLRDHUP;
    }

    if(one_shot){
        event.events |= EPOLLONESHOT;
    }
    //添加到epollfd中
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    //再设置非阻塞
    setnonblocking(fd);
}

//信号处理函数，这里只向管道写,不处理逻辑,写入的是信号的名字，处理逻辑的代码在主程序中
void Utils::sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    //传输的是字符类型
    send(u_pipedfd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    //信号处理函数只发送信号,设置restart在系统调用被中断时能够保证重启该系统调用
    sa.sa_handler = handler;
    if(restart){
        sa.sa_flags |= SA_RESTART;
    }
    //将所有信号添加到信号集中
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::timer_handler(){
    m_timer_lst.tick();//定时处理
    alarm(m_TIMESLOT);//根据超时时间定时发送alarm信号
}

void Utils::show_error(int connfd, const char * info){
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int * Utils::u_pipedfd = 0;
int Utils::u_epollfd = 0;

class Utils;
//回调，进行事件取消监听、关闭文件描述符和减少连接数（记得加）
void cb_func(client_data * user_data){
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    // http_conn::m_user_count--
}