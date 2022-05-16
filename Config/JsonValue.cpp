

#include <assert.h>
#include <string>
#include "JsonValue.h"
#include "JsonParse.h"


namespace cocolay{

namespace m_Json{


void Value::init(const Value& obj){
    m_type = obj.m_type;
    switch(m_type){
    case m_Json::NUMBER:
    {
        m_num = obj.m_num;
        break;
    }
    case m_Json::STRING:
    {
        //placement new，替换new，在m_str的地址处就地创建一个string对象，对象的值为obj.m_string（这种方式既可以在堆上创建，也可以在栈上创建）
        new(&m_str) std::string(obj.m_str);
        break;
    }
    case m_Json::ARRAY:
    {
        new(&m_arr) std::vector<Value>(obj.m_arr);
        break;
    }
    case m_Json::OBJECT:
    {
        new(&m_str) std::vector<std::pair<std::string, Value>>(obj.m_obj);
        break;
    }
    }
}

//显示调用析构函数来释放，采用placement new手段生成的不要使用delete
void Value::free() noexcept {
    using std::string;
    switch (m_type)
    {
    case STRING:
    {
        m_str.~string();
        break;
    }
    case ARRAY:
    {
        m_arr.~vector<Value>();
        break;
    }
    case OBJECT:
    {
        m_obj.~vector<std::pair<std::string, Value>>();
        break;
    }
    }
}


m_Json::JSON_TYPE Value::get_type() const noexcept {
    return m_type;
}

void Value::set_type(m_Json::JSON_TYPE t) noexcept {
    free();
    m_type = t;
}

Value& Value::operator=(const Value &obj) noexcept {
    free();
    init(obj);
    return *this;
}
Value::Value() noexcept {
    m_num = 0;
}

Value::~Value() noexcept {
    free();
}

double Value::get_num() const noexcept {
    assert(m_type == NUMBER);
    return m_num;
}

void Value::set_num(double d) noexcept {
    free();
    m_type = NUMBER;
    m_num = d;
}

const std::string & Value::get_str() const noexcept {
    assert(m_type == STRING);
    return m_str;
}

void Value::set_str(const std::string & str) noexcept {
    if(m_type == STRING){
        m_str = str;
    }
    else{
        free();
        m_type = STRING;
        new(&m_str) std::string(str);
    }
}

size_t Value::get_arr_size() const noexcept {
    assert(m_type == ARRAY);
    return m_arr.size();
}

const Value & Value::get_arr_element(size_t index) const noexcept {
    assert(m_type == ARRAY);
    return m_arr[index];
}

void Value::set_arr(const std::vector<Value>& arr) noexcept {
    if(m_type == ARRAY){
        m_arr = arr;
    }
    else{
        free();
        m_type = ARRAY;
        new(&m_arr) std::vector<Value>(arr);
    }
}

void Value::arr_pushback(const Value & val) noexcept {
    assert(m_type == ARRAY);
    m_arr.push_back(val);
}

void Value::arr_popback() noexcept {
    assert(m_type == ARRAY);
    m_arr.pop_back();
}

void Value::arr_clear() noexcept {
    assert(m_type == ARRAY);
    m_arr.clear();
}

void Value::arr_insert(const Value & val, size_t index) noexcept {
    assert(m_type == ARRAY);
    m_arr.insert(m_arr.begin()+index, val);
}

void Value::arr_erase(size_t index, size_t count) noexcept {
    assert(m_type == ARRAY);
    m_arr.erase(m_arr.begin()+index, m_arr.begin()+index+ count);
}

size_t Value::get_obj_size() const noexcept {
    assert(m_type == OBJECT);
    return m_obj.size();
}

const std::string& Value::get_obj_key(size_t index) const noexcept {
    assert(m_type == OBJECT);
    return m_obj[index].first;
}

const Value & Value::get_obj_value(size_t index) const noexcept {
    assert(m_type == OBJECT);
    return m_obj[index].second;
}

void Value::set_obj_value(const std::string & key, const Value & val) noexcept {
    assert(m_type == OBJECT);
    auto index = obj_find_index(key);
    if(index>=0)m_obj[index].second = val;
    else m_obj.push_back(std::make_pair(key,val));
}

long long Value::obj_find_index(const std::string & key) const noexcept {
    assert(m_type == OBJECT);
    for(int i=0;i<m_obj.size();++i){
        if(m_obj[i].first == key){
            return i;
        }
    }
    return -1;
}

void Value::set_obj(const std::vector<std::pair<std::string,Value>>& obj) noexcept {
    if(m_type == OBJECT){
        m_obj = obj;
    }
    else{
        free();
        m_type = OBJECT;
        new(&m_obj) std::vector<std::pair<std::string,Value>>(obj);
    }
}

void Value::obj_remove_value(size_t index) noexcept {
    assert(m_type == OBJECT);
    m_obj.erase(m_obj.begin() + index, m_obj.begin() + index + 1);
}

void Value::obj_clear() noexcept {
    assert(m_type == OBJECT);
    m_obj.clear();
}

bool operator==(const Value& obj_1, const Value& obj_2) noexcept {
    if(obj_1.m_type != obj_2.m_type)return false;
    switch(obj_1.m_type){
    case STRING:
    {
        return obj_1.m_str == obj_2.m_str;
    }
    case NUMBER:
    {
        return obj_1.m_num == obj_2.m_num;
    }
    case ARRAY:
    {
        return obj_1.m_arr == obj_2.m_arr;
    }
    case OBJECT:
    {
        if(obj_1.get_obj_size() != obj_2.get_obj_size()){
            return false;
        }
        for(size_t i=0;i<obj_1.get_obj_size();++i){
            auto index = obj_2.obj_find_index(obj_1.get_obj_key(i));
            if(index<0 || obj_1.get_obj_value(i) != obj_2.get_obj_value(index) )return false;
        }
        return true;
    }

    }
    return true;
}

bool operator!=(const Value& obj_1, const Value& obj_2) noexcept {
    return !(obj_1 == obj_2);
}

size_t Value::get_obj_key_size(size_t index) const noexcept {
    assert(m_type == OBJECT);
    return m_obj[index].first.size();
}

void Value::parse(const std::string &content){
    Parser(*this, content);
}

}
}