#include<iostream>
#include<stdarg.h>
#include<cstring>

using namespace std;



void sum(const char* msg, ...)
{
    va_list vaList;                        //定义一个va_list型的变量

    // va_start(vaList, msg);                 //va_start初始化vaList
    // int first = va_arg(vaList, int);       //va_arg获取第一个参数
    // char* second = va_arg(vaList, char*);  //va_arg获取第二个参数
    // int third = va_arg(vaList, int);       //va_arg获取第三个参数
    // va_end(vaList);                        //va_end结束vaList，将vaList置NULL

    char str[256]{0};
    va_start(vaList, msg);
    vsprintf(str, msg, vaList); //配合格式化字符串，输出可变参数
    va_end(vaList);
    printf("%s", str);

    return;
}
 
 int main()
 {
     sum("hello world %d %s %d\n", 9, "666", 3);
     return 0;
 }
