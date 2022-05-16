#ifndef __JSON_H__
#define __JSON_H__
#include <string>
#include <memory>



namespace cocolay{
    //json的类型，将boolean单独列为两种
    namespace m_Json{
        enum JSON_TYPE : int {Null, TRUE, FALSE, NUMBER, STRING, ARRAY, OBJECT};
        class Value;
    }
    class Json final{
    public:

        void parse(const std::string& content, std::string& status) noexcept;
        void parse(const std::string& content);

        Json() noexcept;
        ~Json() noexcept;
        //拷贝构造
        Json(const Json& obj) noexcept;
        //重载等号
        Json& operator=(const Json& obj) noexcept;
        //移动构造
        Json(Json&& obj) noexcept;
        //重载等号
        Json& operator=(Json&& obj) noexcept;

    public:
        m_Json::JSON_TYPE get_type() const noexcept;
        void set_null() noexcept;
        void set_boolean(bool b) noexcept;
        Json& operator=(bool b) noexcept {set_boolean(b); return *this;}

        //设置number和获得number
        double get_num() const noexcept;
        void set_num(double d) noexcept;
        Json& operator=(double d) noexcept {set_num(d); return *this;}

        //设置string和获得string
        const std::string& get_str() const noexcept;
        void set_str(const std::string& str) noexcept;
        Json& operator=(const std::string& str) noexcept {set_str(str); return *this;}

        //数组
        size_t get_arr_size() const noexcept;
        Json get_arr_element(size_t index) const noexcept;
        void set_arr() noexcept;
        void arr_pushback(const Json& val) noexcept;
        void arr_popback() noexcept;
        void arr_clear() noexcept;
        void arr_insert(const Json& val, size_t index) noexcept;
        void arr_erase(size_t index, size_t count) noexcept;

        //对象
        size_t get_obj_size() const noexcept;
        const std::string& get_obj_key(size_t index) const noexcept;
        size_t get_obj_key_size(size_t index) const noexcept;
        Json get_obj_value(size_t index) const noexcept;
        void set_obj_value(const std::string& key, const Json& val) noexcept;
        void set_obj() noexcept;
        long long obj_find_index(const std::string& key) const noexcept;
        void obj_remove_value(size_t index) noexcept;
        void obj_clear() noexcept;
    private:

        std::unique_ptr<m_Json::Value> v;
        friend bool operator==(const Json& obj_1, const Json& obj_2) noexcept;
        friend bool operator!=(const Json& obj_1, const Json& obj_2) noexcept;

    };
    bool operator==(const Json& obj_1, const Json& obj_2) noexcept;
    bool operator!=(const Json& obj_1, const Json& obj_2) noexcept;



}

#endif