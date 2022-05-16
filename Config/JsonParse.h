#ifndef __JSONPARSE_H__
#define __JSONPARSE_H__


#include "Json.h"
#include "JsonValue.h"

namespace cocolay{



namespace m_Json
{
class Parser final {
public:
    Parser(Value& val, const std::string &content);

private:
    void parse_whitespace() noexcept;
    void parse_value();
    void parse_literal(const char* literal, m_Json::JSON_TYPE t);
    void parse_num();
    void parse_str();
    void parse_str_raw(std::string& tmp);
    void parse_hex4(const char* &p, unsigned &u);
    void parse_encode_utf8(std::string &str, unsigned u) const noexcept;
    void parse_arr();
    void parse_obj();

private:
    Value& m_val;
    const char * m_cur;

};

} // namespace m_Json end

}// namespace cocolay end











#endif