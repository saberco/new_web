#include"../log/log.h"
#include<unistd.h>

locker mutex;
static int count = 0;
int m_close_log = 0;

void * workinfo(void* args){
    while(1){
        usleep(1000);
        mutex.lock();
        LOG_INFO("INFO: %d", ++count);
        mutex.unlock();
    }
}

void * workwarn(void* args){
    while(1){
        usleep(1000);
        mutex.lock();
        LOG_WARN("WARN: %d", ++count);
        mutex.unlock();
    }
}


void testtongbu(){
    Log::GetInstence()->init("log_test", 0);
    LOG_DEBUG("debug test");
    LOG_INFO("%d, %s\n", 22, "abc");
}




int main(){
    Log::GetInstence()->init("log_test_yibu", 0, 60, 5000, 20);


    pthread_t info,warn;
    pthread_create(&info, NULL,workinfo,NULL);
    pthread_create(&warn, NULL,workwarn,NULL);
    sleep(1);
    pthread_cancel(info);
    pthread_cancel(warn);
    return 0;
}