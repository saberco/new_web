#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__
//线程池封装，主要分为两个部分，一个是工作队列，一个是存放线程的容器

#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include"../lock/lock.h"
#include"../sqlconnpool/sqlconnpool.h"

//为了泛用性，将线程池定义为模板类，可以兼容多种工作请求

template<typename T>
class threadpool{
public:
    //初始化数据库连接池和线程数量、最多允许的请求量
    threadpool(int actor_model,connection_pool* connpool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T* request, int state);
    bool append_p(T* request);
private:
    //工作函数，从请求队列中不断取出任务并且处理之
    static void* worker(void* arg);
    void run();
private:
    int m_thread_number;        //线程池中线程的数量
    int m_max_requests;         //请求队列中允许的最大请求数目
    pthread_t * m_threads;      //线程池数组，大小为m_thread_number
    std::list<T*> m_workqueue;  //请求队列
    locker m_queuelocker;       //保护请求队列的锁
    sem m_queuestat;            //请求队列中是否有任务
    connection_pool* m_connPool; //数据库连接池
    int m_actor_model;          //模型切换,1是reactor模式，0是proactor模式
};

template<typename T>
threadpool<T>::threadpool(int actor_model,connection_pool * connpool, int thread_number, int max_request)
                        :m_thread_number(thread_number)
                        ,m_max_requests(max_request)
                        ,m_threads(NULL)
                        ,m_connPool(connpool)
                        ,m_actor_model(actor_model)
{
    if(thread_number <= 0 || max_request <= 0){
        throw std::exception();
    }
    //堆中创建线程池
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads){
        throw std::exception();
    }
    for(int i=0;i<m_thread_number;++i){
        //由于work是静态成员函数不能访问非静态成员变量，所以需要将this指针作为参数传递给work
        if(pthread_create(m_threads + i, NULL, worker, this) != 0){
            delete [] m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i])){
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool(){
    delete[] m_threads;
}

//给请求队列添加请求
template<typename T>
bool threadpool<T>::append(T* request, int state){
    m_queuelocker.lock();
    //工作队列中的请求过多时，解锁返回false
    if(m_workqueue.size() >= m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    //信号量加1
    m_queuestat.post();
    return true;
}


template<typename T>
bool threadpool<T>::append_p(T* request){
    m_queuelocker.lock();
    //工作队列中的请求过多时，解锁返回false
    if(m_workqueue.size() >= m_max_requests){
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    //信号量加1
    m_queuestat.post();
    return true;
}
//线程工作函数,通过传进来的this调用run
template<typename T>
void * threadpool<T>::worker(void * arg){
    threadpool * pool = (threadpool*) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run(){
    while(true){
        m_queuestat.wait();
        //如果有任务了
        m_queuelocker.lock();
        //再次判断是否有任务,为了线程同步
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        //取出一个
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        //取完解锁
        m_queuelocker.unlock();
        if(!request){
            continue;
        }
        if(1 == m_actor_model){
            if(0 == request->m_state){
                if(request->read_once()){
                    request->improv = 1;
                    //建立一个数据库连接
                    connectionRAII mysqlconn(&request->mysql, m_connPool);
                    //去往真正的HTTP处理请求部分
                    request->process();
                }
                else{
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else{
                if(request->write()){
                    request->improv = 1;
                }
                else{
                    request->improv = 1;
                    request->timer_flag=1;
                }
            }
        }
        else{
            //建立一个数据库连接
            connectionRAII mysqlconn(&request->mysql, m_connPool);
            //去往真正的HTTP处理请求部分
            request->process();
        }
    }
}

#endif