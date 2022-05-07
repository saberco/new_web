#include"../log/log.h"



int main(){
    int m_close_log = 0;
    Log::GetInstence()->init("log_test", 0);
    LOG_DEBUG("debug test");
    LOG_INFO("%d, %s\n", 22, "abc");
    return 0;
}