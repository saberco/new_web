#ifndef __SQLCONNPOOL_H__
#define __SQLCONNPOOL_H__
//数据库连接池的封装

#include<stdio.h>
#include<list>
#include<mysql/mysql.h>
#include<error.h>
#include<string.h>
#include<string>
#include<iostream>
#include"../lock/lock.h"

class connection_pool{
public:
    MYSQL* GetConnection();             //获取一个连接
    bool ReleaseConnection(MYSQL* conn);//释放一个连接
    int GetFreeConn();                  //获取当前空闲连接数
    void DestroyPool();                 //销毁数据库连接池

    //池采用单例模式
    static connection_pool* GetInstence();
    connection_pool();
    ~connection_pool();

    void init(std::string url, std::string User, std::string PassWord, std::string DatabaseName, int Port, unsigned int MaxConn, int close_log);

private:
    std::string url;                         //主机地址
    std::string Port;                        //数据库端口号
    std::string User;                        //登录数据库的用户名
    std::string PassWord;                    //登录数据库用的密码
    std::string DatabaseName;                //数据库名
    int m_close_log;                         //日志开关
private:
    locker lock;
    sem reserve;
    std::list<MYSQL *> connList;             //数据库连接池

private:
    unsigned int MaxConn;               //最大连接数
    unsigned int CurConn;               //当前连接数
    unsigned int FreeConn;              //当前空闲的连接数

};

//自动内存管理类
class connectionRAII{
public:
    connectionRAII(MYSQL **con, connection_pool* connPool);
    ~connectionRAII();

private:
    MYSQL* conRAII;
    connection_pool * poolRAII;

};



#endif