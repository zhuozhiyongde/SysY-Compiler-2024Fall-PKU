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
 * @note - `type`：符号的类型，变量 VAR / 常量 VAL / 数组 ARR / 指针 PTR
 * @note - `value`：符号的值
 * @note    1. 对于 ARR 和 PTR 类型，value 表示数组或指针的维度
 * @note    2. 对于 VAR 和 VAL 类型，value 表示变量或常量的值
 */
class Symbol {
public:
    enum class Type {
        VAR,
        VAL,
        ARR,
        PTR
    };
    Type type;
    // 对于 ARR 和 PTR 类型，value 表示数组或指针的维度
    int value;
    Symbol() : type(Type::VAL), value(0) {}
    Symbol(Type type, int value = 0) : type(type), value(value) {}
};

/**
 * @brief 一些宏定义，用于快速创建 Symbol 对象
 */

#define VAR_ Symbol(Symbol::Type::VAR, 0)
#define VAL_(value) Symbol(Symbol::Type::VAL, value)
#define ARR_(value) Symbol(Symbol::Type::ARR, value)
#define PTR_(value) Symbol(Symbol::Type::PTR, value)

 /**
  * @brief 符号表类，管理变量名和符号，实现为单向链表
  * @note - `symbol_table`：符号表，存储变量名和符号
  * @note - `depth`：符号表的深度
  * @note - `is_returned`：包含符号表是否已经存在 return 语句，用于判断是否需要生成后续语句
  * @note - `parent`：指向父级符号表的指针
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
    string locate(const string& ident);
    string assign(const string& ident);
};

/**
 * @brief 结果类，表示前端生成的指令的结果
 * @note - `type`：结果的类型，常量 IMM / 寄存器 REG
 * @note - `value`：结果的值
 * @note    1. 对于 IMM 类型，value 表示常量的值
 * @note    2. 对于 REG 类型，value 表示寄存器的编号
 */
class Result {
public:
    enum class Type {
        IMM,
        REG
    };
    Type type;
    int value;
    // 友元函数，用于打印 Result 对象
    friend ostream& operator<<(ostream& os, const Result& result) {
        os << (result.type == Type::REG ? "%" : "") << result.value;
        return os;
    }
    // 构造函数
    Result() : type(Type::IMM), value(0) {}
    Result(Type type, int value) : type(type), value(value) {}
};

/**
 * @brief 一些宏定义，用于快速创建 Result 对象
 */

#define IMM_(value) Result(Result::Type::IMM, value)
#define REG_(value) Result(Result::Type::REG, value)

 /**
  * @brief 前端全局信息管理器，管理横跨 block 的全局信息
  * @note - `if_else_count`：if-else 语句的计数，用于生成 if-else 语句的标签
  * @note - `while_count`：while 语句的计数，用于生成 while 语句的标签
  * @note - `while_current`：当前正在处理的 while 语句标号（break/continue 使用）
  * @note - `short_circuit_count`：短路求值的计数，用于生成短路求值的标签
  * @note - `jump_count`：跳转语句的计数，用于生成跳转语句的标签
  * @note - `is_symbol_allocated`：是否已经存在过分配某变量的指令，避免重复 alloc
  * @note - `is_global`：当前是否为全局，用于控制 Decl 语句的生成
  * @note - `is_func_return`：函数是否有返回值，用于控制 Call 语句的生成
  * @note - `temp_count`：临时变量分配计数，用于生成临时变量，SSA 静态单赋值使用
  */
class EnvironmentManager {
private:
    int if_else_count = 0;
    int short_circuit_count = 0;
    // 包括 ret/break/continue 三种跳转语句
    int jump_count = 0;
    // while 最大标号（生成 label）
    int while_count = 0;
    // 当前正在处理的 while 语句标号（break/continue）
    int while_current = 0;
public:
    // 当前是否为全局，用于控制 Decl 语句的生成
    bool is_global = true;
    // 临时变量分配计数
    int temp_count = 0;
    // 是否已经存在过分配某变量的指令，避免重复 alloc
    unordered_map<string, bool> is_symbol_allocated;
    // 函数是否有返回值，用于控制 Call 语句的生成
    unordered_map<string, bool> is_func_return;

    // if-else 语句

    string get_then_label();
    string get_else_label();
    string get_end_label();

    // while 语句

    string get_while_entry_label(bool current = false);
    string get_while_end_label(bool current = false);
    string get_while_body_label();

    // 短路求值

    string get_short_true_label();
    string get_short_false_label();
    string get_short_end_label();
    string get_short_result_reg();

    // 跳转语句

    string get_jump_label();

    // 操作私有变量

    void add_if_else_count();
    void add_while_count();
    void add_short_circuit_count();
    void set_while_current(int current);
    int get_temp_count();
    int get_while_count();
    int get_while_current();
};

extern EnvironmentManager environment_manager;

/**
 * @brief 一些宏定义，用于快速创建 SSA 寄存器
 */

#define NEW_REG_ REG_(environment_manager.temp_count++)
#define CUR_REG_ REG_(environment_manager.temp_count - 1)

extern ofstream koopa_ofs;

void init_lib();
void format_array_type(const vector<int>& indices);
void print_array_type(const string& ident, const vector<int>& indices);
void print_array(const string& ident, const vector<int>& indices, int* array, int level, int& index, vector<int>& bases);
