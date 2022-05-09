#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>



Log::Log(){
    m_count = 0;
    m_is_async = false;
}

Log::~Log(){
    if(m_fp!=nullptr){
        fclose(m_fp);
    }
}
//通过是否设置阻塞队列长度区分同步日志与异步日志，同步日志不需要阻塞队列
bool Log::init(const char* file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size){
    //如果设置了阻塞队列大小，说明为异步
    if(max_queue_size > 0){
        m_is_async = true;
        m_log_queue = new block_queue<std::string>(max_queue_size);
        pthread_t tid;
        //创建一个新的线程进行异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }

    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    m_split_lines = split_lines;
    //给日志缓冲区设置大小并填充为字符串结束符
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    //结构体tm保存了时间量
    time_t t = time(NULL);
    struct tm * sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //找到路径中的文件名,从后往前找第一个'/'
    const char *p = strrchr(file_name,'/');
    char log_full_name[256] = {0};
    //NULL的意思表示整个字符串就是文件名
    if(p==NULL){
        //文件重命名
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year+1900,my_tm.tm_mon+1,my_tm.tm_mday,file_name);
    }
    else{
        //移动到/后边，即去掉/，然后复制到log_name中
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255,"%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year+1900,my_tm.tm_mon+1,my_tm.tm_mday,log_name);
    }
    m_today = my_tm.tm_mday;
    //append方式绝对路径打开日志文件
    m_fp = fopen(log_full_name, "a");
    if(m_fp == nullptr){
        return false;
    }
    return true;
}

//分级写入文件
void Log::write_log(int level, const char *format,...){
    //获取现在的时间到my_tm结构体
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    //s为日志格式
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[error]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }

    //写入一个log，将增加行数
    m_mutex.lock();
    m_count++;
    //如果不是今天的日志或者日志的行数太多了，就分开
    if(m_today != my_tm.tm_mday || m_count % m_split_lines == 0){
        char new_log[256] = {0};
        //将缓冲区强制写入文件并关闭现在的文件
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year+1900,my_tm.tm_mon+1,my_tm.tm_mday);
        //如果是非今日日志导致的
        if(m_today!= my_tm.tm_mday){
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        
        }else{
            //如果是日志行数超行导致的
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        //打开新的文件
        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();
    //采用va宏处理可变参数
    va_list valst;
    va_start(valst, format);

    std::string log_str;
    m_mutex.lock();

    //写入具体的时间格式
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    //内容格式化，用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数(不包含终止符)
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    //这里可能会数组越界，判断一下
    // m_buf[n + m] = '\n';
    // m_buf[n + m + 1] = '\0';
    if(n + m + 1 > m_log_buf_size - 1){
        m_buf[m_log_buf_size-2] = '\n';
        m_buf[m_log_buf_size-1] = '\0';
    }
    else{
        m_buf[n + m] = '\n';
        m_buf[n + m + 1] = '\0';
    }
    log_str = m_buf;

    m_mutex.unlock();

    //当数据在m_buf缓冲区了，判断采用同步写或是异步写,异步就放到阻塞队列，同步就直接写
    if(m_is_async && !m_log_queue->full()){
        m_log_queue->push(log_str);
    }
    else{
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);

}

void Log::flush(void){
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}