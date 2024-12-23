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

/**
 * @brief 符号类，表示变量或常量
 * @note - 包含符号的类型（type，变量 VAR / 常量 VAL）
 * @note - 包含符号的值（value）
 */
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

/**
 * @brief 符号表类，管理变量名和符号，实现为单向链表
 * @note - 包含符号表的深度（depth）
 * @note - 包含符号表是否已经存在 return 语句（is_returned），用于判断是否需要生成后续语句
 * @note - 包含符号表的父符号表（parent）
 */
class SymbolTable {
private:
    unordered_map<string, Symbol> symbol_table;
public:
    int depth = 0;
    bool is_returned = false;
    SymbolTable* parent = nullptr;

    void create(const string& ident, Symbol symbol);
    bool exist(const string& ident);
    Symbol read(const string& ident);
    void set_parent(SymbolTable* parent);
    // 从当前层符号表中递归向上查找变量名
    string locate(const string& ident);
    // 在当前层符号表中分配变量名
    string assign(const string& ident);
};

/**
 * @brief 结果类，表示前端生成的指令的结果
 * @note - 包含结果的类型（type，常量 IMM / 寄存器 REG）
 * @note - 包含结果的值（value）
 */
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

/**
 * @brief 前端上下文管理器，管理横跨 block 的局部信息
 * @note - 管理是否已经存在过分配某变量的指令，避免重复 alloc
 * @note - 管理 if-else 语句的计数，用于生成 if-else 语句的标签
 * @note - 管理 return 语句的计数，用于生成 return 语句的标签
 * @note - 管理临时变量的分配
 */
class FrontendContextManager {
private:
    int if_else_count = 0;
    int ret_count = 0;
    int temp_count = 0;
public:
    unordered_map<string, bool> is_symbol_allocated;
    string get_then_label();
    string get_else_label();
    string get_end_label();
    string get_ret_end_label();
    void add_if_else_count();
    int get_temp_count();
};

#define NEW_REG_(fcm) REG_(fcm.get_temp_count())