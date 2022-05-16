#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include "JsonException.h"
#include "JsonParse.h"

namespace cocolay{
namespace m_Json{
    
inline void expect(const char* &c, char ch){
    //断言，并让c指针指向下一个地址
    assert(*c == ch);
    ++c;
}


Parser::Parser(Value & val, const std::string & content)
    :m_val(val), m_cur(content.c_str())
{
    m_val.set_type(m_Json::Null);
    parse_whitespace();
    parse_value();
    parse_whitespace();
    if(*m_cur !='\0'){
        m_val.set_type(m_Json::Null);
        throw(Exception("parse root not singular"));
    }

}

//解析空格、空行，操作为直接跳过
void Parser::parse_whitespace() noexcept {
    while(*m_cur == ' ' || *m_cur == '\t' || *m_cur == '\n' || *m_cur == '\r')++m_cur;
}

//解析值
void Parser::parse_value(){
    switch(*m_cur){
    case 'n':
    {
        parse_literal("null", m_Json::Null);
        return;
    } 
    case 't':
    {
        parse_literal("true", m_Json::TRUE);
        return;
    }   
    case 'f':
    {
        parse_literal("false", m_Json::FALSE);
        return;
    }  
    case '\"':
    {
        parse_str();
        return;
    }
    case '[':
    {
        parse_arr();
        return;
    }
    case '{':
    {
        parse_obj();
        return;
    }   
    case '\0' :{
        throw(Exception("parse expect value"));
    }
    default:{
        parse_num();
        return;
    } 
    }
}



//解析bool和null类型
void Parser::parse_literal(const char * literal, m_Json::JSON_TYPE t) {
    expect(m_cur, literal[0]);
    size_t i;
    //遇见'\0'停止运行
    for(i=0;literal[i+1];++i){
        if(m_cur[i]!=literal[i+1]){
            throw (Exception("parse invalid value literal"));
        }
    }
    m_cur += i;
    m_val.set_type(t);
}

void Parser::parse_num() {
    const char*p = m_cur;
    //负数和前缀0,因为序列化的json是字符串形式
    if(*p == '-')++p;
    if(*p == '0')++p;
    else{    
        if(!isdigit(*p)) throw (Exception("parse invalid value num"));
        while(isdigit(*++p));
    }
    if(*p == '.'){
        if(!isdigit(*++p)) throw (Exception("parse invalid value num"));
        while(isdigit(*++p));
    }
    //支持科学计数法
    if(*p == 'e' || *p == 'E'){
        ++p;
        if(*p=='+'||*p=='-')++p;
        if(!isdigit(*p))throw (Exception("parse invalid value num"));
        while(isdigit(*++p)) ;
    }
    errno = 0;
    //直接转化为浮点数
    double v = strtod(m_cur, NULL);
    //大数错误
    if(errno == ERANGE && (v==HUGE_VAL || v == -HUGE_VAL)){
        throw (Exception("parse number too big"));
    }
    m_val.set_num(v);
    //指针移动到下一个需要解析的开头
    m_cur = p;
}

void Parser::parse_str() {
    std::string s;
    //s是传出参数
    parse_str_raw(s);
    m_val.set_str(s);
}

void Parser::parse_str_raw(std::string & tmp) {
    expect(m_cur, '\"');
    //已经跳过了第一个"
    const char *p = m_cur;
    unsigned u=0, u2=0;
    //遇到结尾引号之前
    while(*p !='\"'){
        if(*p == '\0')throw (Exception("parse miss quotation mark"));
        if(*p == '\\' && ++p){
            switch(*p++){
            case '\"':
            {
                tmp+='\"';
                break;
            }
            case '/':
            {
                tmp+='/';
                break;
            }
            case '\\':
            {
                tmp+='\\';
                break;
            }
            case 'b':
            {
                tmp+='\b';
                break;
            } 
            case 'f':
            {
                tmp+='\f';
                break;
            } 
            case 'n':
            {
                tmp+='\n';
                break;
            } 
            case 'r':
            {
                tmp+='\r';
                break;
            } 
            case 't':
            {
                tmp+='\t';
                break;
            }      
            //处理unicode转义
            case 'u':
            {
                //传出参数u，解析到的存在u中,如果解析到的范围在(0xD800-0xDBFF)那么需要进行两次解析，
                //一次是高位代理(0xD800-0xDBFF)，一次是低位代理(0xDC00-0xDFFF)
                //变(H,L)公式为codepoint = 0x10000 + ((H − 0xD800) << 10) | (L − 0xDC00)
                parse_hex4(p, u);
                if(u>=0xD800 && u<=0xDBFF){
                    if(*p++ != '\\')throw(Exception("parse invalid unicode surrogate"));
                    if(*p++ != 'u')throw(Exception("parse invalid unicode surrogate"));
                    parse_hex4(p, u2);
                    if(u2<0xDC00 || u2> 0xDFFF)throw(Exception("parse invalid unicode surrogate"));
                     u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                }
                parse_encode_utf8(tmp, u);

            }
            default:{
                throw (Exception("parse invalid string escape"));
            }
            }
        }
        else if((unsigned char)*p < 0x20){
            throw (Exception("parse invalid string char"));
        }
        //其他情况直接解引用
        else tmp += *p++;
    }
    m_cur = ++p;
}



void Parser::parse_hex4(const char *& p, unsigned & u) {
    u = 0;
    //unicode是四位16进制
    for(int i=0;i<4;++i){
        char ch=*p++;
        u<<4;
        if(isdigit(ch)){
            u |= ch-'0';
        }
        else if(ch>='A' && ch<='F'){
            u |= ch - ('A' - 10);
        }
        else if(ch>='a' && ch<='f'){
            u |= ch - ('a' - 10);
        }
        else throw(Exception("parse invalid unicode hex"));
    }
}

//根据utf8编码表实现，将编码转化为字符
void Parser::parse_encode_utf8(std::string & str, unsigned u) const noexcept {
    //按位与是为了避免编译器警告
    if (u <= 0x7F)
        str += static_cast<char> (u & 0xFF);
    else if (u <= 0x7FF) {
        str += static_cast<char> (0xC0 | ((u >> 6) & 0xFF));
        str += static_cast<char> (0x80 | ( u	   & 0x3F));
    }
    else if (u <= 0xFFFF) {
        str += static_cast<char> (0xE0 | ((u >> 12) & 0xFF));
        str += static_cast<char> (0x80 | ((u >>  6) & 0x3F));
        str += static_cast<char> (0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        str += static_cast<char> (0xF0 | ((u >> 18) & 0xFF));
        str += static_cast<char> (0x80 | ((u >> 12) & 0x3F));
        str += static_cast<char> (0x80 | ((u >>  6) & 0x3F));
        str += static_cast<char> (0x80 | ( u        & 0x3F));
    }
}



void Parser::parse_arr() {
    expect(m_cur, '[');
    parse_whitespace();
    std::vector<Value> tmp;
    if(*m_cur == ']'){
        ++m_cur;
        m_val.set_arr(tmp);
        return ;
    }
    //此中的m_val只是解出的每一个类型，最后会变为数组
    for(;;){
        try{
            parse_value();
        }catch(Exception) {
            m_val.set_type(m_Json::Null);
            throw;
        }
        tmp.push_back(m_val);
        parse_whitespace();
        if(*m_cur==','){
            ++m_cur;
            parse_whitespace();
        }
        else if(*m_cur == ']'){
            ++m_cur;
            m_val.set_arr(tmp);
            return ;
        }
        else{
            m_val.set_type(m_Json::Null);
            throw(Exception("parse miss comma or square bracket arr"));
        }
    }
}

//解析对象
void Parser::parse_obj() {
    expect(m_cur, '{');
    std::vector<std::pair<std::string, Value>> tmp;
    std::string key;
    parse_whitespace();
    if(*m_cur == '}'){
        ++m_cur;
        m_val.set_obj(tmp);
        return;
    }
    //for(;;)比while(true)性能更高
    for(;;){
        //每个key一定是字符串
        if(*m_cur != '\"'){
            throw(Exception("parse miss key"));
        }
        try{
            parse_str_raw(key);
        }catch(Exception) {
            throw(Exception("parse key error"));
        }
        parse_whitespace();
        if(*m_cur++ != ':')throw(Exception("parse miss colon"));
        parse_whitespace();
        try{
            parse_value();
        }catch(Exception) {
            m_val.set_type(m_Json::Null);
            throw;
        }
        tmp.push_back(std::make_pair(key,m_val));
        m_val.set_type(m_Json::Null);
        key.clear();
        parse_whitespace();
        if(*m_cur==','){
            ++m_cur;
            parse_whitespace();
        }else if(*m_cur=='}'){
            ++m_cur;
            m_val.set_obj(tmp);
            return;
        }else{
            m_val.set_type(m_Json::Null);
            throw(Exception("parse miss comma or square bracket obj"));
        }
    }
}












}
}