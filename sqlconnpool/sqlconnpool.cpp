
//数据库连接池的实现
#include<mysql/mysql.h>
#include<stdio.h>
#include<string>
#include<string.h>
#include<list>
#include<pthread.h>
#include<stdlib.h>
#include<iostream>
#include"sqlconnpool.h"

//构造和析构
connection_pool::connection_pool(){
    this->CurConn = 0;
    this->FreeConn = 0;
}
connection_pool::~connection_pool(){
    DestroyPool();
}

int connection_pool::GetFreeConn(){
    return this->FreeConn;
}

//局部静态变量的懒汉单例模式
connection_pool * connection_pool::GetInstence(){
    static connection_pool connPool;
    return &connPool;
}
//初始化函数
void connection_pool::init(std::string url, std::string User, std::string PassWord, std::string DatabaseName, int Port, unsigned int MaxConn,int close_log){
    this->url = url;
    this->User = User;
    this->PassWord = PassWord;
    this->DatabaseName = DatabaseName;
    
    //操作线程池，由于是共享资源，需要加锁
    lock.lock();
    for(int i=0;i<MaxConn;++i){
        MYSQL* con = nullptr;
        con = mysql_init(con);
        //证明初始化失败了
        if(con==nullptr){
            std::cout<< "Error :" << mysql_error(con);
            exit(1);
        }
        con = mysql_real_connect(con, url.c_str(), User.c_str(),PassWord.c_str(), DatabaseName.c_str(),Port, NULL,0);
        //证明连接失败了
        if(con==nullptr){
            std::cout<< "Error :" << mysql_error(con);
            exit(1);
        }
        connList.push_back(con);
        ++FreeConn;
    }

    reserve = sem(FreeConn);
    this->MaxConn = FreeConn;
    lock.unlock();
}

//从连接池中获取一个连接
MYSQL * connection_pool::GetConnection(){
    MYSQL* con = nullptr;

    if(0 == connList.size()){
        return nullptr;
    }
    //信号量减少一个
    reserve.wait();
    lock.lock();

    con = connList.front();
    connList.pop_front();

    --FreeConn;
    ++CurConn;

    lock.unlock();

    return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL * conn){
    if(nullptr == conn){
        return false;
    }

    lock.lock();

    connList.push_back(conn);
    ++FreeConn;
    --CurConn;

    lock.unlock();

    reserve.post();

    return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool(){
    lock.lock();
    if(connList.size() > 0){
        std::list<MYSQL*>::iterator it;
        for(it = connList.begin();it!=connList.end();++it){
            MYSQL* con = *it;
            mysql_close(con);
        }
        CurConn = 0;
        FreeConn = 0;
        connList.clear();
    }
    lock.unlock();
}

//RAII管理
connectionRAII::connectionRAII(MYSQL ** con, connection_pool * connPool){
    *con = connPool -> GetConnection();

    conRAII = *con;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
    poolRAII -> ReleaseConnection(conRAII);
}
