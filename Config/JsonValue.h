#ifndef __JSONVALUE_H__
#define __JSONVALUE_H__

#include <string>
#include <vector>
#include <utility>
#include "Json.h"

namespace cocolay{
namespace m_Json{


    class Value final {
    public:
        Value() noexcept;
        ~Value() noexcept;
        //拷贝构造
        Value(const Value& obj) noexcept{init(obj);}
        //重载等号必须返回引用，为了防止连等错误，等号会自动调用拷贝构造函数，如果是浅拷贝，会导致错误
        Value& operator=(const Value &obj) noexcept;

    public:
        //解析
        void parse(const std::string &content);
        //获得JSON类型
        m_Json::JSON_TYPE get_type() const noexcept;
        //设置JSON类型
        void set_type(m_Json::JSON_TYPE t) noexcept;
        //设置number和获得number
        double get_num() const noexcept;
        void set_num(double d) noexcept;

        //设置string和获得string
        const std::string& get_str() const noexcept;
        void set_str(const std::string& str) noexcept;

        //数组相关
        size_t get_arr_size() const noexcept;
        const Value& get_arr_element(size_t index) const noexcept;
        void set_arr(const std::vector<Value> &arr) noexcept;
        void arr_pushback(const Value& val) noexcept;
        void arr_popback() noexcept;
        void arr_clear() noexcept;
        void arr_insert(const Value& val, size_t index) noexcept;
        void arr_erase(size_t index, size_t count) noexcept;

        //对象相关
        size_t get_obj_size() const noexcept;
        const std::string& get_obj_key(size_t index) const noexcept;
        size_t get_obj_key_size(size_t index) const noexcept;
        const Value& get_obj_value(size_t index) const noexcept;
        void set_obj_value(const std::string& key, const Value& val) noexcept;
        void set_obj(const std::vector<std::pair<std::string, Value>> &obj) noexcept;
        long long obj_find_index(const std::string& key) const noexcept;
        void obj_remove_value(size_t index) noexcept;
        void obj_clear() noexcept;


    private:
        void init(const Value& obj);
        void free() noexcept;

    private:
        cocolay::m_Json::JSON_TYPE m_type;
        //一个对象只可能是一种类型
        union {
            double m_num;
            std::string m_str;
            std::vector<Value> m_arr;
            std::vector<std::pair<std::string, Value>> m_obj;
        };

        friend bool operator==(const Value& obj_1, const Value& obj_2) noexcept;

    };

    //为了方便调用，可以将操作符重载为友元函数方便操作类的私有成员，而防止不进行成员函数的调用引起错误,友元重载的参数多余成员函数参数
    bool operator==(const Value& obj_1, const Value& obj_2) noexcept;
    bool operator!=(const Value& obj_1, const Value& obj_2) noexcept;
}
}



#endif