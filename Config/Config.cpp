#include"Config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <iostream>


Config::Config(){
    m_buf = (char*)malloc(sizeof(char) *1000);
    int fd = open("/home/coco/new_web/WebConfig.json", O_RDONLY);
    int ret = read(fd, m_buf ,1000);
    if(ret<0){
        printf("error");
        exit(0);
    }
    m_json.parse(m_buf);
    int st = m_json.get_obj_size();
    for(int i=0;i<st;++i){
        auto key = m_json.get_obj_key(i);
        auto v = m_json.get_obj_value(i);
        m_config[key] =  v.get_num();
    }
    for(auto num:m_config){
        if(num.first=="PORT")PORT = num.second;
        else if(num.first=="LOGWrite") LOGWrite = num.second;
        else if(num.first=="TRIGMode")TRIGMode = num.second;
        else if(num.first=="LISTENTRIGMode")LISTENTRIGMode = num.second;
        else if(num.first=="CONNTRIGMode")CONNTRIGMode = num.second;
        else if(num.first=="OPT_LINGER")OPT_LINGER = num.second;
        else if(num.first=="sql_num")sql_num = num.second;
        else if(num.first=="thread_num")thread_num = num.second;
        else if(num.first=="close_log")close_log = num.second;
        else if(num.first=="actor_model")actor_model = num.second;
    }
}

