#include"http_conn.h"

#include<mysql/mysql.h>
#include<fstream>

//http响应的一些状态码信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

//锁
locker m_lock;
map<string, stirng> users;
//初始化静态变量
int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

void http_conn::initresultFile(connection_pool * connPool){
    //从连接池中获取一个连接
    MYSQL* mysql = nullptr;
    connectionRAII(mysql, connPool);

    //从表中检索username和passwd数据
    if(mysql_query(mysql, "SELECT name, pwd from users")){
        LOG_ERROR("SELECT error : %s\n", mysql_error(mysql));
    }

    //获取结果集
    MYSQL_RES * result = mysql_store_result(mysql);
    //结果集的列数
    int num_fields = mysql_num_fields(result);
    //返回所有字段结构的数组
    MYSQL_FIELD* fields = mysql_fetch_fields(result);

    //从结果集中一行一行获取并存入Map
    while(MYSQL_ROW row = mysql_fetch_row(result)){
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp[2];
    }
}

//epoll相关
//设置文件描述符非阻塞
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//添加epoll监听事件到epollfd,不用添加listenfd,只针对socketfd
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode){
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

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//删除epoll文件描述符的监听
void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//重新注册fd到epollfd，因为oneshot会导致fd从监听事件中移除，需要重新添加
void modfd(int epollfd, int fd, int ev, int TRIGMode){
    epoll_event event;
    event.data.fd = fd;

    if(1 == TRIGMode){
        event.events = ev | EPOLLONESHOT | EPOLLET | EPOLLRDHUP;
    }
        
    else{
        event.events =  ev | EPOLLONESHOT  | EPOLLRDHUP;
    }

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

//关闭一个连接
void http_conn::close_conn(bool real_close){
    if(real_close && (m_sockfd != -1)){
        printf("close fd : %d", m_sockfd);
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

//初始化连接，外部调用来初始化套接字文件描述符
void http_conn::init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMode, int close_log, string user, string passwd, string sqlname){
    m_sockfd = sockfd;
    m_address = addr;
    doc_root = root;
    m_TRIGMode = TRIGMode;
    m_close_log = close_log;
    addfd(m_epollfd, m_sockfd, true, m_TRIGMode);

    strcpy(sql_user, user.c_str());
    strcpy(sql_passwd, passwd.c_str());
    strcpy(sql_name, sqlname.c_str());
}

//内部调用初始化
void http_conn::init(){
    mysql = nullptr;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

//一次性读取发送来的http请求
bool http_conn::read_once(){
    if(m_read_idx >= READ_BUFFER_SIZE)return false;

    int bytes_read = 0;
    //LT读取
    if(0 == m_TRIGMode){
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytes_read;

        if(bytes_read<=0)return false;
        return true;
    }
    //ET读取需要一次性读完所有数据，直到无数据可读或者对方关闭连接
    else{
        while(true){
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if(bytes_read == -1){
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if(bytes_read == 0){
                return false;
            }
            //修改字符数继续读，直到读完
            m_read_idx += bytes_read;
        }
        return true;
    }
}

//线程调用process函数进行处理，主要是调用process_read和process_write两个函数进行http请求解析和写入http响应报文
void http_conn::process(){
    HTTP_CODE read_ret = process_read();
    //NO_request表示请求不完整，需要继续请求接收数据
    if(read_ret == NO_REQUEST){
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        return;
    }
    //调用process_write写http响应
    bool write_ret = process_write(read_ret);
    if(!write_ret){
        close_conn();
    }
    //重新注册并监听写事件
    modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
}

//主状态机，对报文的每一行进行循环处理，
HTTP_CODE http_conn::process_read(){
    //初始化从状态机和http请求的解析结果
    LINE_STATE line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;

    //两个判断条件，
    while((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK)){
        //get_line获取解析的行的首地址
        text = get_line();

        //m_start_line 是每一个数据行在m_read_buf的起始位置
        //m_checked_idx是从状态机在m_read_buf中读取的位置，处理完一行需要将其置到主状态机中
        m_start_line = m_checked_idx;

        //主状态机的三种状态转移
        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            //解析请求首行
            ret = parse_request_line(text);
            if(ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER:
        {
            //解析请求头
            ret = parse_headers(text);
            if(ret == GET_REQUEST){
                //完整解析GET请求后，跳转到报文响应函数
                return do_request();
            }
            else if(ret == BAD_REQUEST){
                return BAD_REQUEST;
            }
            break;
        } 
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if(ret == GET_REQUEST){
                return do_request();
            }
            //解析完消息体，避免重入循环更改行状态
            line_status = LINE_OPEN;
            break;
        }   
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

//从状态机，负责找到每一个完整的行,找\r\n，返回值是LINE_OK，读取到一行完整的，LINE_OPEN还需要继续接收，LINE_BAD语法有问题 
//m_read_idx指向m_read_buf中数据末尾的下一节，m_checked_idx为当前正在分析的字符位  
LINE_STATE http_conn::parse_line(){
    char temp;
    for(;m_checked_idx < m_read_idx; ++m_checked_idx){
        //temp为当前要分析的字符
        temp = m_read_buf[m_checked_idx];
        //如果当前字符位/r，判断下一个字符
        if(temp == '\r'){
            //如果是数据末尾，则不完整，需要继续接收
            if((m_checked_idx + 1) == m_read_idx){
                return LINE_OPEN;
            }
            //如果是\n，则返回LINE_OK
            else if(m_read_buf[m_checked_idx+1] == '\n'){
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }//其他情况是错误的
            return LINE_BAD;
        }//如果当前字符是\n一般是上次读取到\r就到了buffer末尾，没有接收完整，再次接收时会出现这种情况）
        else if(temp == '\n'){
            //判断前一个字符是否是'\r'
            if(m_checked_idx>1&&m_read_buf[m_checked_idx-1] == '\r'){
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    //没有找到/r需要继续接收
    return LINE_OPEN;
}

//主状态机的三种逻辑
//在此之前，从状态机已经将一行的\r\n转变为\0\0字符串结束符
//首先是请求首行
HTTP_CODE http_conn::parse_request_line(char * text){
    //请求首行包含了需要访问的资源url，所使用的的http版本，请求类型
    //最先含有空格和\t是请求方法
    m_url = strpbrk(text, " \t");
    if(!m_url)return BAD_REQUEST;
    //将当前位置改为'\0'，方便取出，然后指向下一个字符
    *m_url++ = '\0';
    //取出请求方法
    char* method = text;
    //只解析get和post
    if(strcasecmp(method, "GET") == 0){
        m_method = GET;
    }
    else if(strcasecmp(method, "POST") == 0){
        m_method = POST;
        cgi = 1;
    }
    else{
        return BAD_REQUEST;
    }
    //然后是要请求的url，继续向后查找,strspn是用来跳过多余的空格\t
    m_url+=strspn(m_url, " \t");

    //这里首先把中间的url结束变为\0
    *m_version = strpbrk(m_url, " \t");
    if(!m_version)return BAD_REQUEST;
    *m_version++ = '\0';
    //这里找到版本,然后是版本仅支持HTTP1.1
    m_version += strspn(m_version, " \t");
    if(strcasecmp(m_version, "HTTP/1.1")!=0)return BAD_REQUEST;

    //对m_url进行处理,首先去掉http://和https://,在让m_url匹配到第一个出现'\'的位置
    if(strncasecmp(m_url, "http://", 7) == 0){
        m_url+=7;
        m_url=strchr(m_url,'/');
    }
    if(strncasecmp(m_url, "https://", 8) == 0){
        m_url+=8;
        m_url=strchr(m_url,'/');
    }

    //如果没有资源或者不是以'\'开头
    if(!m_url || m_url[0]!='/'){
        return BAD_REQUEST;
    }

    //具体的资源路径,如果是/则显示主页面，其他则按照原本的来
    if(strlen(m_url) == 1){
        strcat(m_url, "judge.html");
    }
    //请求头已经解析完毕，改变状态
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;

}

//然后是解析请求头
HTTP_CODE http_conn::parse_headers(char * text){
    //解析到空行说明请求头解析完毕，判断contentlength是否为0，判断是否需要解析请求体
    //首先进行判断，是空行还是请求头
    if(text[0] == '\0'){
        //判断是否存在请求体
        if(m_content_length != 0){
            //需要转移到请求体解析状态
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }//不是空行则进行其他的解析
    else if(strncasecmp(text, "Connection:", 11) == 0){
        text +=11;
        //跳过空格和\t
        text+=strspn(text, " \t");
        //如果是长连接，设置连接保持
        if (strcasecmp(text, "keep-alive") == 0){
            m_linger = true;
        }
        
    }
    else if(strncasecmp(text, "Content-length:", 15) == 0){
        text +=15;
        //跳过空格和\t
        text+=strspn(text, " \t");
        m_content_length = atol(text);
    }
    else if(strncasecmp(text, "Host:", 5) == 0){
        text +=15;
        //跳过空格和\t
        text+=strspn(text, " \t");
        m_host = text;
    }
    else{
        LOG_INFO("oop!unknow header: %s",text);
    }
    return NO_REQUEST;
}


//解析请求体
HTTP_CODE http_conn::parse_content(char * text){
    //判断是否读取了消息体
    if(m_read_idx >= (m_content_length + m_checked_idx)){
        text[m_content_length] = '\0';
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}