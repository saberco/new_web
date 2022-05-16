
#include "Json.h"
#include <vector>
#include "JsonValue.h"
#include"JsonException.h"

namespace cocolay{

//unique指针需要普通指针进行初始化
Json::Json() noexcept : v(new m_Json::Value){
    
}

Json::~Json() noexcept{}

Json::Json(const Json& obj) noexcept{
    //指向新的对象，释放原来的空间
    v.reset(new m_Json::Value(*(obj.v)));
    
}

//实现移动语义,相当于对原来的指针先get再reset()
Json::Json(Json&& obj) noexcept{
    v.reset(obj.v.release());
}

Json& Json::operator=(const Json& obj) noexcept{
    v.reset(new m_Json::Value(*(obj.v)));
    return *this;
}

Json& Json::operator=(Json&& obj) noexcept{
    v.reset(obj.v.release());
    return *this;
}

cocolay::m_Json::JSON_TYPE Json::get_type() const noexcept{
    if(v == nullptr)return m_Json::Null;
    return v->get_type();
}

void Json::set_null() noexcept {
    v->set_type(m_Json::Null);
}

void Json::set_boolean(bool b) noexcept {
    if(b) v->set_type(m_Json::TRUE);
    else v->set_type(m_Json::FALSE);
}

double Json::get_num() const noexcept {
    return v->get_num();
}

void Json::set_num(double d) noexcept {
    v->set_num(d);
}

const std::string & Json::get_str() const noexcept {
    return v->get_str();
}

void Json::set_str(const std::string & str) noexcept {
    v->set_str(str);
}

size_t Json::get_arr_size() const noexcept {
    return v->get_arr_size();
}

Json Json::get_arr_element(size_t index) const noexcept {
    Json ret;
    ret.v.reset(new m_Json::Value(v->get_arr_element(index)));
    return ret;
}

void Json::set_arr() noexcept{
    v->set_arr(std::vector<m_Json::Value>{});
}

void Json::arr_pushback(const Json& val) noexcept {
    v->arr_pushback(*val.v);
}
void Json::arr_popback() noexcept {
    v->arr_popback();
}

void Json::arr_clear() noexcept {
    v->arr_clear();
}

void Json::arr_insert(const Json& val, size_t index) noexcept {
    v->arr_insert(*val.v, index);
}

void Json::arr_erase(size_t index, size_t count) noexcept {
    v->arr_erase(index, count);
}

size_t Json::get_obj_size() const noexcept {
    return v->get_obj_size();
}

//const修饰返回值返回指针，代表指针内容不能被修改，只能附加给被const修饰的同类指针
//修饰返回值返回引用，主要是限制不能将函数调用作为左值，只能去赋值
const std::string& Json::get_obj_key(size_t index) const noexcept {
    return v->get_obj_key(index); 
}


size_t Json::get_obj_key_size(size_t index) const noexcept {
    return v->get_obj_key_size(index);
}

Json Json::get_obj_value(size_t index) const noexcept {
    Json ret;
    ret.v.reset(new m_Json::Value(v->get_obj_value(index)));
    return ret;
}

void Json::set_obj_value(const std::string& key, const Json& val) noexcept {
    v->set_obj_value(key, *val.v);
}

void Json::set_obj() noexcept {
    v->set_obj(std::vector<std::pair<std::string, m_Json::Value>>{});
}

long long Json::obj_find_index(const std::string& key) const noexcept{
    return v->obj_find_index(key);
}

void Json::obj_remove_value(size_t index) noexcept {
    v->obj_remove_value(index);
}

void Json::obj_clear() noexcept {
    v->obj_clear();
}

bool operator==(const Json& obj_1, const Json& obj_2) noexcept {
    return *obj_1.v == *obj_2.v;
}

bool operator!=(const Json& obj_1, const Json& obj_2) noexcept {
    return *obj_1.v != *obj_2.v;
}

void Json::parse(const std::string& content, std::string& status) noexcept {
    try{
        parse(content);
        status = "parse ok";
    }catch(const m_Json::Exception & msg){
        status = msg.what();
    }catch(...){

    }
}

void Json::parse(const std::string& content) {
    v->parse(content);
}

}