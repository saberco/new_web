#ifndef __HTTPCONN_H__
#define __HTTPCONN_H__
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
#include <map>

#include"../lock/lock.h"
#include"../sqlconnpool/sqlconnpool.h"
// #include"../timer/timerlst.h"
#include "../timer/heaptimer.h"
#include"../log/log.h"


class http_conn{
public:
    static const int FILENAME_LEN = 200;            //文件名称大小
    static const int READ_BUFFER_SIZE = 2048;       //读缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024;      //写缓冲区大小
    //请求报文的方法
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH};
    //主状态机调用从状态机读取到完整的一行请求后，
    //按照HTTP请求的结构判断该行属于请求行还是头部字段，
    //然后进行分别解析，直到遇到一个空行，表明得到了一个正确的http请求
    //主状态机状态
    enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};
    //从状态机状态OK代表读完整请求，bad代表读错请求，open代表还没读完请求
    enum LINE_STATE {LINE_OK = 0, LINE_BAD, LINE_OPEN};
    //HTTP状态码
    enum HTTP_CODE {
        NO_REQUEST,         //http请求不完整
        GET_REQUEST,        //获得了完整的http请求
        BAD_REQUEST,        //http请求有错误
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,     //服务器内部错误
        CLOSER_CONNECTION
    };
public:
    http_conn(){}
    ~http_conn(){}

public:
    //初始化
    void init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMode, int close_log , std::string user, std::string passwd, std::string sqlname);
    //关闭http连接
    void close_conn(bool real_close = true);
    //工作函数
    void process();
    //一次性读取客户端发来的所有数据
    bool read_once();
    //写入响应报文
    bool write();
    //获取结构体
    sockaddr_in* get_address(){
        return &m_address;
    }

    //CGI使用线程池初始化数据表
    void initresultFile(connection_pool* connPool);
    int timer_flag;
    int improv;

private:
    void init();
    //从m_read_buf读取并处理请求报文
    HTTP_CODE process_read();
    //向m_write_buf写入响应报文
    bool process_write(HTTP_CODE ret);
    //主状态机解析请求行
    HTTP_CODE parse_request_line(char* text);
    //主状态机解析请求头
    HTTP_CODE parse_headers(char* text);
    //主状态机解析请求体
    HTTP_CODE parse_content(char* text);


    //生成响应报文
    HTTP_CODE do_request();
    //get_line用来将指针后移指向没有处理的字符
    char* get_line() {return m_read_buf + m_start_line;}
    //从状态机读取一行，分析是解析的哪一部分
    LINE_STATE parse_line();
    void unmap();

    //生成对应8个部分的响应报文，由do_request进行调用
    bool add_response(const char* format,...);
    bool add_content(const char* content);
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL* mysql;
    int m_state;        //读为0，写为1

private:
    int m_sockfd;
    sockaddr_in m_address;

    char m_read_buf[READ_BUFFER_SIZE];          //读缓冲区，存储读取的请求报文信息
    int m_read_idx;                             //读取缓冲区中数据的最后一个字节的下一个位置
    int m_checked_idx;                          //读缓冲区的位置
    int m_start_line;                           //读缓冲区已经解析的字符

    char m_write_buf[WRITE_BUFFER_SIZE];        //存储响应报文数据
    int m_write_idx;                            //指示写缓冲区长度

    CHECK_STATE m_check_state;                  //主状态机状态
    METHOD m_method;                            //请求方法

    //对应请求报文中的请求数据
    char m_real_file[FILENAME_LEN];
    char* m_url;
    char* m_version;
    char* m_host;
    int m_content_length;
    bool m_linger;

    //和写http响应报文有关的成员
    char* m_file_address;                       //http请求的文件在服务器上的地址
    struct stat m_file_stat;                    //所请求的文件信息，文件大小，创建时间访问时间等
    struct iovec m_iv[2];                       //iovec向量，用于分散写，即将响应报文和内存中的文件一起写入写缓冲区，生成http响应，响应体就是html部分
    int m_iv_count;

    int cgi;                                    //是否使用post
    char *m_string;                             //请求头的数据
    int bytes_to_send;                          //剩余发送字节数
    int bytes_have_send;                        //已发送字节数
    char* doc_root;

    std::map<std::string,std::string> m_users;                 //用户名密码映射
    int m_TRIGMode;                             //epoll触发方式
    int m_close_log;                            //是否启用日志

    char sql_user[100];                         //连接数据库用的用户名
    char sql_passwd[100];                       //密码
    char sql_name[100];                         //数据库名
};

#endif