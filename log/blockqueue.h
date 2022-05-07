#ifndef __BLOCKQUEUE_H__
#define __BLOCKQUEUE_H__

//阻塞队列，当队列为空时，获取元素的线程会等待队列变为非空，当队列为满时，存储元素的队列会等待队列可用
//采用循环数组实现阻塞队列,为了线程安全，除了构造函数，其他操作都是先加锁然后操作
#include<iostream>
#include<stdlib.h>
#include<pthread.h>
#include<sys/time.h>
#include"../lock/lock.h"

template<typename T>
class block_queue{
public:
    //构造
    block_queue(int max_size = 1000){
        if(max_size <= 0){
            exit(-1);
        }

        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }
    //析构
    ~block_queue(){
        //加锁避免多次析构
        m_mutex.lock();
        if(m_array != NULL){
            delete [] m_array;
        }
        m_mutex.unlock();
    }
    //clear方法
    void clear(){
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }
    //判断队列是否已满
    bool full(){
        m_mutex.lock();
        if(m_size >= m_max_size){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    //判断队列是否为空
    bool empty(){
        m_mutex.lock();
        if(0 == m_size){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    //返回队首元素
    bool front(T & value){
        m_mutex.lock();
        if(0 == m_size){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    //返回队尾元素
    bool back(T & value){
        m_mutex.lock();
        if(0 == m_size){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }
    //返回当前队列大小
    int size(){
        int tmp = 0;
        m_mutex.lock();
        tmp = m_size;
        m_mutex.unlock();
        return tmp;
    }
    //返回当前队列容量
    int max_size(){
        int tmp = 0;
        m_mutex.lock();
        tmp = m_max_size;
        m_mutex.unlock();
        return tmp;
    }
    //阻塞队列主要功能，空时取出操作阻塞，满时放入操作阻塞
    //放入操作，如果有线程等待，broadcast可以唤醒等待的线程，如果没有那么broadcast操作无意义
    bool push(const T& item){
        m_mutex.lock();
        if(m_size>=m_max_size){
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;

        m_size++;

        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }

    bool pop(T& item){
        m_mutex.lock();
        while(m_size<=0){
            //如果队列中没有元素，就阻塞等待条件变量唤醒，等待的过程中是解锁状态，唤醒后重新加锁
            if(!m_cond.wait(m_mutex.get())){
                m_mutex.unlock();
                return false;
            }
        }
        m_front = (m_front+1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

    //增加了超时的pop
    bool pop(T& item, int ms_timeout){
        struct timespec t = {0,0};
        struct timeval now = {0,0};
        gettimeofday(&now, NULL);
        m_mutex.lock();
        if(m_size <= 0){
            t.tv_sec = now.tv_sec + ms_timeout /1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if(!m_cond.time_wait(m_mutex.get(), t)){
                m_mutex.unlock();
                return false;
            }
        }
        //万一被取走了
        if(m_size<=0){
            m_mutex.unlock();
            return false;
        }
        m_front = (m_front+1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex;
    cond m_cond;

    T* m_array;
    int m_size;
    int m_max_size;     //阻塞队列最大size
    int m_front;
    int m_back;
};





#endif