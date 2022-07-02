
#include "heaptimer.h"
#include"../http_conn/http_conn.h"




void Heap_timer::add_timer(util_timer * timer){
    if(timer->id <0)return;
    size_t i;
    //如果是新节点，插入队列然后调整队列
    if(!m_ref.count(timer->id)){
        i = m_heap.size();
        m_ref[timer->id] = i;
        m_heap.push_back(timer);
        siftup_(i);
    }
}

void Heap_timer::adjust_timer(util_timer * timer){
    if(m_ref.count(timer->id) && !m_heap.empty()){
        siftdown_(m_ref[timer->id], m_heap.size());
    }
    
}


void Heap_timer::del_timer(util_timer * timer){
    if(!m_ref.count(timer->id) || m_heap.empty())return;
    //将要删除的节点调整到队尾然后调整堆
    size_t i = m_ref[timer->id];
    size_t n = m_heap.size() - 1;
    assert(i<=n);
    if(i<n){
        swapnode_(i, n);
        if(!siftdown_(i,n)){
            siftup_(i);
        }
    }
    //删除队尾
    m_ref.erase(m_heap.back()->id);
    m_heap.pop_back();
    delete timer;
}

void Heap_timer::del_(size_t index){
    assert(index>=0);
    if(m_heap.empty())return;
    size_t i = index;
    size_t n = m_heap.size() - 1;
    assert(i<=n);
    if(i<n){
        swapnode_(i, n);
        if(!siftdown_(i,n)){
            siftup_(i);
        }
    }
    //删除队尾
    auto tmp = m_heap.back();
    m_ref.erase(tmp->id);
    m_heap.pop_back();
    delete tmp;
}

void Heap_timer::tick(){
    time_t cur = time(NULL);
    if(m_heap.empty())return;
    while(!m_heap.empty()){
        auto node = m_heap.front();
        if(node->expire > cur)break;
        node->cb_func(node->user_data);
        pop();
    }
}


void Heap_timer::pop(){
    if(m_heap.empty())return;
    del_(0);
}

void Heap_timer::siftup_(size_t index){
    assert(index>=0);
    if(index>=m_heap.size())return;
    size_t tmp = (index-1) / 2;
    //这是与父节点做比较，如果当前节点大于父节点直接break，如果当前节点小于父节点，需要沿着路径进行交换，当前节点可能与父节点交换视为up
    while(tmp>=0){
        if(m_heap[tmp] < m_heap[index])break;
        swapnode_(index, tmp);
        index = tmp;
        tmp = (index-1) / 2;
    }
}

void Heap_timer::swapnode_(size_t index_i, size_t index_j){
    assert(index_i>=0&& index_i<m_heap.size());
    assert(index_j>=0&& index_j<m_heap.size());
    std::swap(m_heap[index_i], m_heap[index_j]);
    m_ref[m_heap[index_i]->id] = index_i;
    m_ref[m_heap[index_j]->id] = index_j;
}

//当前节点可能与子节点进行交换视为down，当子节点大于父节点时，执行，若不成功则执行up
bool Heap_timer::siftdown_(size_t index, size_t n){
    assert(index>=0&& index<m_heap.size());
    assert(n>=0&& n<=m_heap.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j<n){
        if(j+1 < n && m_heap[j+1] <m_heap[j])++j;
        if(m_heap[i] < m_heap[j])break;
        swapnode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void Heap_timer::clear_(){
    m_heap.clear();
    m_ref.clear();
}

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
    //******************
    m_heap_timer.tick();//定时处理
    //********************
    alarm(m_TIMESLOT);//根据超时时间定时发送alarm信号
}


void Utils::show_error(int connfd, const char * info){
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int * Utils::u_pipedfd = 0;
int Utils::u_epollfd = 0;


void cb_func(client_data * user_data){
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}