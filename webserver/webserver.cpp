#include"webserver.h"

Webserver::Webserver(){

    //HTTP_CONN类对象数组
    users = new http_conn[MAX_FD];

    //资源文件夹路径，工作路径要在new_web路径下
    char server_path[50] = "/home/coco/boke/myblog";
    
    char source[8] = "/public";
    m_root = (char*)malloc(strlen(server_path) + strlen(source) + 1);

    strcpy(m_root, server_path);
    strcat(m_root, source);

    //定时器对象数组
    users_timer = new client_data[MAX_FD];

}
//析构
Webserver::~Webserver(){
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[0]);
    close(m_pipefd[1]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}

//进行初始化
void Webserver::init(int port, std::string user, std::string password, std::string databaseName, 
              int log_write, int opt_linger, int TRIGMode, int sql_num,
              int thread_num, int close_log, int actor_model)
{
    m_port = port;
    m_user = user;
    m_password = password;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_log_write = log_write;
    m_thread_num = thread_num;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = TRIGMode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

//创建线程池
void Webserver::thread_pool(){
    m_pool = new threadpool<http_conn>(m_actormodel,m_connPool,m_thread_num);
}
//创建数据库连接池
void Webserver::sql_pool(){
    m_connPool = connection_pool::GetInstence();
    m_connPool->init("localhost", m_user, m_password, m_databaseName, 3306, m_sql_num, m_close_log);

    //读取数据库表到内存
    users->initresultFile(m_connPool);
}

//初始化日志系统
void Webserver::log_write(){
    if(0 == m_close_log){
        if( 1 == m_log_write){
            //异步
            Log::GetInstence()->init("./ServerLog", m_close_log, 2000, 800000, 800);
        }
        else{
            //同步
            Log::GetInstence()->init("./ServerLog", m_close_log, 2000, 800000, 0);
        }
    }
}

//设置epoll触发模式
void Webserver::trig_mode(){
    //主要是设置Listen的问题
    //lt+lt
    if( 0 == m_TRIGMode){
        m_LISTENTRIGMode = 0;
        m_CONNTRIGMode = 0;
    }//lt+et
    else if(1 == m_TRIGMode){
        m_LISTENTRIGMode = 0;
        m_CONNTRIGMode = 1;
    }//et+lt
    else if(2 == m_TRIGMode){
        m_LISTENTRIGMode = 1;
        m_CONNTRIGMode = 0;
    }//et+et
    else if(3 == m_TRIGMode){
        m_LISTENTRIGMode = 1;
        m_CONNTRIGMode = 1;
    }
}
//开启epoll监听
void Webserver::eventListen(){
    //网络编程基础步骤，创建监听-》端口复用-》绑定-》监听-》接收连接-》传递
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    //关闭连接(优雅)，调用close会在未发送数据发送完成后退出
    if(0 == m_OPT_LINGER){
        struct linger tmp = {0,1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }//关闭连接(优雅)，设置超时时间，如果越过超时时间没有得到确认，会强制退出
    else if (1 == m_OPT_LINGER){
        struct linger tmp = {1,1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);    //本地IP
    address.sin_port = htons(m_port);
    
    int flag = 1;
    //设置端口复用
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    //绑定
    ret = bind(m_listenfd,(struct sockaddr*)&address, sizeof(address));
    assert(ret>=0);
    //监听
    ret = listen(m_listenfd, 5);
    assert(ret>=0);

    //初始化定时器
    utils.init(TIMESLOT);
    //epoll的创建和注册
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    //添加定时器监听
    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTRIGMode);
    http_conn::m_epollfd = m_epollfd;

    //管道，创建两个相连的socket
    ret = socketpair( PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret!=-1);
    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    utils.addsig(SIGPIPE, SIG_IGN);
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);

    //定时产生信号
    alarm(TIMESLOT);

    Utils::u_pipedfd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

//初始化定时器
void Webserver::timer(int connfd, sockaddr_in client_address){
    //users是http连接数组，初始化这个链接，在每一次进行连接的时候进行调用
    users[connfd].init(connfd,client_address, m_root, m_CONNTRIGMode, m_close_log, m_user, m_password, m_databaseName);

    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，并绑定用户，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer* timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    //TIMESLOT为5s
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    utils.m_heap_timer.add_timer(timer);
}

//若出现了数据传输，则延后定时器
void Webserver::adjust_timer(util_timer * timer){
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_heap_timer.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}

void Webserver::deal_timer(util_timer * timer, int sockfd){
    timer->cb_func(&users_timer[sockfd]);
    if(timer){
        utils.m_heap_timer.del_timer(timer);
    }

    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

//处理用户数据
bool Webserver::dealclientdata(){
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    //lt
    if(0 == m_LISTENTRIGMode){
        int connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &client_addrlength);
        if(connfd < 0){
            LOG_ERROR("%s:errno is : %d", "accept error", errno);
            return false;
        }
        if(http_conn::m_user_count >= MAX_FD){
            //告诉客户端服务器正忙
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        //添加文件描述符到定时器
        timer(connfd, client_address);
    }
    //et
    else{
        while(1){
            int connfd = accept(m_listenfd, (struct sockaddr*)&client_address, &client_addrlength);
            if(connfd < 0){
                LOG_ERROR("%s:errno is : %d", "accept error", errno);
                break;
            }
            if(http_conn::m_user_count >= MAX_FD){
                //告诉客户端服务器正忙
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            //添加文件描述符到定时器
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

//处理定时器信号,参数是传出参数
bool Webserver::dealwithsignal(bool &timeout, bool &stop_server){
    int ret = 0;
    int sig;
    char signals[1024];
    //从管道读出信号值，成功返回字节数，失败返回-1
    //正常情况下ret返回1，主要是14和15对应的两个ASCII码
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if(ret == -1){
        return false;
    }
    else if(ret ==0){
        return false;
    }
    else{
        //处理信号的逻辑
        for(int i=0;i<ret;++i){
            switch(signals[i]){
                case SIGALRM:{
                    timeout = true;
                    break;
                }
                case SIGTERM:{
                    stop_server = true;
                    break;
                }
            }
        }
    }
    return true;
}

//读用户发送的数据
void Webserver::dealwithread(int sockfd){
    util_timer* timer = users_timer[sockfd].timer;
    //reactor
    if(1 == m_actormodel){
        if(timer){
            adjust_timer(timer);
        }
        //检测到读事件，将该事件放入请求队列
        m_pool->append(users + sockfd, 0);
        while(true){
            if(1 == users[sockfd].improv){
                if(1 == users[sockfd].timer_flag){
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else{
        //proactor
        if(users[sockfd].read_once()){
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));//地址转化为点分十进制
            //检测到读事件放入请求队列
            m_pool->append_p(users + sockfd);
            if(timer){
                adjust_timer(timer);
            }
        }else{
            deal_timer(timer, sockfd);
        }
    }
}


//写数据给用户
void Webserver::dealwithwrite(int sockfd){
    util_timer* timer = users_timer[sockfd].timer;
    //reactor
    if(1==m_actormodel){
        if(timer){
            adjust_timer(timer);
        }
        m_pool->append(users+sockfd, 1);
        while(true){
            if(1 == users[sockfd].improv){
                if(1 == users[sockfd].timer_flag){
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    //proactor
    else{
        if(users[sockfd].write()){
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
            if(timer){
                adjust_timer(timer);
            }
        }
        else{
            deal_timer(timer, sockfd);
        }
    }
}


void Webserver::eventLoop(){
    bool timeout = false;
    bool stop_server = false;

    while(!stop_server){
        //等待epoll事件，返回的是事件并存入events数组
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if(number<0&&errno!=EINTR){
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for(int i=0;i<number;++i){
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if(sockfd == m_listenfd){
                bool flag = dealclientdata();
                if(false == flag){
                    continue;
                }
            }//如果是异常的事件
            else if(events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)){
                //关闭并移除
                util_timer* timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            //如果是定时器信号
            else if((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN)){
                //接收到定时器信号
                bool flag = dealwithsignal(timeout, stop_server);
                if(false == flag){
                    LOG_ERROR("%s" , "dealclientdata failure");
                }
            }
            else if(events[i].events & EPOLLIN){
                dealwithread(sockfd);
            }
            else if(events[i].events & EPOLLOUT){
                dealwithwrite(sockfd);
            }
        }
        if(timeout){
            utils.timer_handler();
            LOG_INFO("%s", "time tick");
            timeout = false;
        }
    }
}