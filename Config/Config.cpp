#include"Config.h"

Config::Config(){
    PORT = 9006;
    //默认同步
    LOGWrite = 0;
    //触发组合模式,默认listenfd LT + connfd LT
    TRIGMode = 0;
    //两个触发模式
    LISTENTRIGMode = 0;
    CONNTRIGMode = 0;

    //优雅关闭连接
    OPT_LINGER = 0;
    //数据库连接池数量
    sql_num = 8;
    //线程数量
    thread_num = 8;
    //默认不关闭日志
    close_log = 0;
    //并发模型默认proactor
    actor_model = 0;
}


void Config::parse_arg(int argc, char * argv[]){
    int opt;
    const char* str = "p:l:m:o:s:t:c:a:";
    while((opt = getopt(argc, argv, str)) != -1){
        switch(opt)
        {
        case 'p':
        {
            PORT = atoi(optarg);
            break;
        }
        case 'l':
        {
            LOGWrite = atoi(optarg);
            break;
        }
        case 'm':
        {
            TRIGMode = atoi(optarg);
            break;
        }
        case 'o':
        {
            OPT_LINGER = atoi(optarg);
            break;
        }
        case 's':
        {
            sql_num = atoi(optarg);
            break;
        }
        case 't':
        {
            thread_num = atoi(optarg);
            break;
        }
        case 'c':
        {
            close_log = atoi(optarg);
            break;
        }
        case 'a':
        {
            actor_model= atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}