#ifndef __JSONEXCEPTION_H__
#define __JSONEXCEPTION_H__

#include<string>
#include<stdexcept>

namespace cocolay{

namespace m_Json{
    class Exception final : public std::logic_error{
    public:
        Exception(const std::string &errmsg) : logic_error(errmsg){}
    };
}
}













#endif