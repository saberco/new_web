#ifndef __LOCK_H__
#define __LOCK_H__

//线程同步机制封装，利用RAII思想，让对象的资源和生命周期保持一直，构造分配，析构回收
#include<pthread.h>
#include<semaphore.h>
#include<exception>

class sem{
public:
    //构造函数初始化信号量
    sem(){
        if(sem_init(&m_sem, 0, 0) != 0){
            throw std::exception();
        }
    }
    sem(int num){
        if(sem_init(&m_sem, 0, num) != 0){
            throw std::exception();
        }
    }
    //析构函数进行资源的回收
    ~sem(){
        sem_destroy(&m_sem);
    }
    //定义PV操作
    bool wait(){
        return sem_wait(&m_sem) == 0;
    }
    bool post(){
        return sem_post(&m_sem) == 0;
    }
private:
    sem_t m_sem;
};

//定义锁类
class locker{
public:
//同信号量
    locker(){
        if(pthread_mutex_init(&m_mutex, NULL)!=0){
            throw std::exception();
        }
    }

    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }
//上锁和解锁操作
    bool lock(){
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock(){
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
//获得锁
    pthread_mutex_t* get(){
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

//条件变量
class cond{
public:
    cond(){
        if(pthread_cond_init(&m_cond, NULL)!=0){
            throw std::exception();
        }
    }

    ~cond(){
        pthread_cond_destroy(&m_cond);
    }

    //条件变量提供的是线程的通知机制，当某个共享数据达到要求时，唤醒等待这个共享数据的线程
    bool signal(){
        return pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast(){
        return pthread_cond_broadcast(&m_cond) == 0;
    }

    bool wait(pthread_mutex_t* m_mutex){
        return pthread_cond_wait(&m_cond, m_mutex);
    }

    bool time_wait(pthread_mutex_t* m_mutex, struct timespec t){
        return pthread_cond_timedwait(&m_cond, m_mutex, &t);
    }

private:
    pthread_cond_t m_cond;

};

#endif