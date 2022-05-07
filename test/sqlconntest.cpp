#include"../sqlconnpool/sqlconnpool.h"
#include<unistd.h>
using namespace std;


// //线程工作函数
void * client1(void* arg){
    MYSQL* my_con = nullptr;
    connectionRAII conRAII(&my_con,connection_pool::GetInstence());
    if(mysql_query(my_con, "SELECT name, pwd FROM users")){
        cout<< "SELECT error: "<< mysql_error(my_con);
    }
    //查询到之后，从表中检索完整的结果集
    MYSQL_RES* result = mysql_store_result(my_con);
    //打印查询结果
    while(MYSQL_ROW row = mysql_fetch_row(result)){
        cout<<"client1->"<<row[0]<<","<<row[1]<<endl;
    }
    return nullptr;
}

void * client2(void* arg){
    MYSQL* my_con = nullptr;
    connectionRAII conRAII(&my_con,connection_pool::GetInstence());
    if(mysql_query(my_con, "SELECT name, pwd FROM users")){
        cout<< "SELECT error: "<< mysql_error(my_con);
    }
    //查询到之后，从表中检索完整的结果集
    MYSQL_RES* result = mysql_store_result(my_con);
    //打印查询结果
    while(MYSQL_ROW row = mysql_fetch_row(result)){
        cout<<"client2->"<<row[0]<<","<<row[1]<<endl;
    }
    return nullptr;
}



//主测试函数
int main(){
    connection_pool* con_pool = connection_pool::GetInstence();
    string url = "localhost";
    string User = "root";
    string pwd = "123456";
    string db = "test";
    con_pool -> init(url, User, pwd, db, 3306, 2, 0);
    MYSQL* my_con = nullptr;
    connectionRAII conRAII(&my_con, con_pool);
    
    if(mysql_query(my_con, "SELECT name, pwd FROM users")){
        cout<< "SELECT error: "<< mysql_error(my_con);
    }
    //查询到之后，从表中检索完整的结果集
    MYSQL_RES* result = mysql_store_result(my_con);
    //打印查询结果
    while(MYSQL_ROW row = mysql_fetch_row(result)){
        cout<<"main->"<<row[0]<<","<<row[1]<<endl;
    }
    sleep(2);
    pthread_t th1,th2;
    pthread_create(&th1, NULL, client1, NULL);
    pthread_detach(th1);
    sleep(2);
    pthread_create(&th2, NULL, client2, NULL);
    pthread_detach(th2);
    sleep(4);
    return 0;
}