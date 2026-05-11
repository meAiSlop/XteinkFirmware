#pragma once

#include <cctype>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mini_json {
struct Value;
using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;

struct Value {
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;
    Storage data;
    bool isObject() const { return std::holds_alternative<Object>(data); }
    bool isArray() const { return std::holds_alternative<Array>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isNumber() const { return std::holds_alternative<double>(data); }
};

class Parser {
  public:
    explicit Parser(const std::string& s) : s_(s) {}
    std::optional<Value> parse() { return parseValue(); }

  private:
    const std::string& s_; size_t i_{0};
    void ws(){ while(i_<s_.size() && std::isspace((unsigned char)s_[i_])) ++i_; }
    bool match(char c){ ws(); if(i_<s_.size()&&s_[i_]==c){++i_; return true;} return false; }
    std::optional<Value> parseValue(){ ws(); if(i_>=s_.size()) return std::nullopt; char c=s_[i_]; if(c=='{') return parseObject(); if(c=='[') return parseArray(); if(c=='"') return parseString(); if(std::isdigit((unsigned char)c)||c=='-') return parseNumber(); if(s_.compare(i_,4,"true")==0){i_+=4; return Value{true};} if(s_.compare(i_,5,"false")==0){i_+=5; return Value{false};} if(s_.compare(i_,4,"null")==0){i_+=4; return Value{nullptr};} return std::nullopt; }
    std::optional<Value> parseString(){ if(!match('"')) return std::nullopt; std::string out; while(i_<s_.size()){ char c=s_[i_++]; if(c=='"') return Value{out}; if(c=='\\' && i_<s_.size()) out.push_back(s_[i_++]); else out.push_back(c);} return std::nullopt; }
    std::optional<Value> parseNumber(){ ws(); size_t start=i_; if(s_[i_]=='-') ++i_; while(i_<s_.size()&&std::isdigit((unsigned char)s_[i_])) ++i_; double d=std::stod(s_.substr(start,i_-start)); return Value{d}; }
    std::optional<Value> parseArray(){ if(!match('[')) return std::nullopt; Array a; ws(); if(match(']')) return Value{a}; while(true){ auto v=parseValue(); if(!v) return std::nullopt; a.push_back(*v); ws(); if(match(']')) return Value{a}; if(!match(',')) return std::nullopt; } }
    std::optional<Value> parseObject(){ if(!match('{')) return std::nullopt; Object o; ws(); if(match('}')) return Value{o}; while(true){ auto k=parseString(); if(!k||!k->isString()) return std::nullopt; if(!match(':')) return std::nullopt; auto v=parseValue(); if(!v) return std::nullopt; o[std::get<std::string>(k->data)] = *v; ws(); if(match('}')) return Value{o}; if(!match(',')) return std::nullopt; } }
};

inline std::optional<Value> parse(const std::string& s){ return Parser(s).parse(); }

} // namespace mini_json
