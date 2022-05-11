#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include"blockqueue.h"

class Log {
public:
    //局部懒汉单例
    
    static Log* GetInstence(){
        static Log instence;
        return &instence;
    }
    //线程工作函数
    static void *flush_log_thread(void* args){
        Log::GetInstence()->async_write_log();
        return nullptr;
    }
    //初始化
    bool init(const char* file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    //将输出内容按标准格式整理
    void write_log(int level, const char *format, ...);
    //强制刷新缓冲区
    void flush(void);
private:
//这里将构造函数设计为私有是避免类外创建
    Log();
    virtual ~Log();
    //异步写日志方法
    void * async_write_log(){
        std::string single_log;
        //从阻塞队列取出一个string，写日志
        while(m_log_queue->pop(single_log)){
            m_mutex.lock();
            fputs(single_log.c_str(),m_fp);
            m_mutex.unlock();
        }
        return nullptr;
    }

private:
    char dir_name[128];             //路径名
    char log_name[128];             //log文件名
    int m_split_lines;              //日志最大行数
    int m_log_buf_size;             //日志缓冲区大小
    long long m_count;              //记录日志行数
    int m_today;                    //日志记录哪天
    FILE* m_fp;                     //打开log文件的指针
    char *m_buf;                    //日志缓冲区
    block_queue<std::string> *m_log_queue;//阻塞队列
    bool m_is_async;                //是否为同步
    locker m_mutex;                 
    int m_close_log;                //关闭日志clear
    

};

//日志级别宏函数定义
//##__VA_ARGS__更加健壮，会自动去掉多余的逗号
#define LOG_DEBUG(format,...) if(0 == m_close_log) {Log::GetInstence()->write_log(0, format, ##__VA_ARGS__); Log::GetInstence()->flush();}
#define LOG_INFO(format,...) if(0 == m_close_log) {Log::GetInstence()->write_log(1, format, ##__VA_ARGS__); Log::GetInstence()->flush();}
#define LOG_WARN(format,...) if(0 == m_close_log) {Log::GetInstence()->write_log(2, format, ##__VA_ARGS__); Log::GetInstence()->flush();}
#define LOG_ERROR(format,...) if(0 == m_close_log) {Log::GetInstence()->write_log(3, format, ##__VA_ARGS__); Log::GetInstence()->flush();}


#endif