#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <optional>
#include <vector>
#include <unordered_map>
#include <cassert>

using namespace std;

class Symbol {
public:
    enum class Type {
        VAR,
        VAL
    };
    Type type;
    int value;
    Symbol() : type(Type::VAL), value(0) {}
    Symbol(Type type, int value = 0) : type(type), value(value) {}
};

#define VAR_ Symbol(Symbol::Type::VAR, 0)
#define VAL_(value) Symbol(Symbol::Type::VAL, value)

class SymbolTable {
private:
    unordered_map<string, Symbol> symbol_table;
    SymbolTable* parent = nullptr;
    int depth = 0;
    bool is_returned = false;
public:
    void create(const string& ident, Symbol symbol);
    bool exist(const string& ident);
    Symbol read(const string& ident);
    void set_parent(SymbolTable* parent);
    void set_depth(int depth);
    // 从当前层符号表中递归向上查找变量名
    string locate(const string& ident);
    // 在当前层符号表中分配变量名
    string assign(const string& ident);
    void set_returned(bool is_returned);
    bool get_returned();
};

class Result {
public:
    enum class Type {
        IMM,
        REG
    };
    Type type;
    int value;
    friend ostream& operator<<(ostream& os, const Result& result) {
        os << (result.type == Type::REG ? "%" : "") << result.value;
        return os;
    }
    Result() : type(Type::IMM), value(0) {}
    Result(Type type, int value) : type(type), value(value) {}
};

#define IMM_(value) Result(Result::Type::IMM, value)
#define REG_(value) Result(Result::Type::REG, value)