#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <optional>
#include <vector>
#include <deque>
#include <unordered_map>
#include <cassert>
#include <typeinfo>
#include "include/frontend_utils.hpp"

using namespace std;

extern string mode;
extern ofstream koopa_ofs;
extern ofstream riscv_ofs;

/**
 * @brief 所有 AST 的基类
 */
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual Result print() const = 0;
};

/**
 * @brief 程序 AST 类
 */
class ProgramAST : public BaseAST {
public:
    //  程序单元，全局变量或函数定义
    vector<unique_ptr<BaseAST>> comp_units;
    Result print() const override;
};

/**
 * @brief 函数定义 AST 类
 */
class FuncDefAST : public BaseAST {
public:
    // 函数类型
    enum class FuncType {
        INT,
        VOID
    };
    FuncType func_type;
    // 函数名
    string ident;
    // 函数参数列表
    vector<unique_ptr<BaseAST>>* func_f_params;
    // 函数体
    unique_ptr<BaseAST> block;
    Result print() const override;
};

/**
 * @brief 函数形式参数 AST 类
 */
class FuncFParamAST : public BaseAST {
public:
    // 参数名
    string ident;
    // 是否为数组参数
    bool is_array;
    // 数组下标
    vector<unique_ptr<BaseAST>>* array_index;
    // 在函数签名内打印
    void as_param() const;
    // 在函数体内打印
    Result print() const override;
};

/**
 * @brief 基本块 AST 类
 */
class BlockAST : public BaseAST {
public:
    // 基本块内容
    vector<unique_ptr<BaseAST>> block_items;
    Result print() const override;
};

/**
 * @brief 常量声明 AST 类
 */
class ConstDeclAST : public BaseAST {
public:
    // 常量定义列表
    vector<unique_ptr<BaseAST>> const_defs;
    Result print() const override;
};

/**
 * @brief 常量定义 AST 类
 */
class ConstDefAST : public BaseAST {
public:
    // 常量名
    string ident;
    // 数组下标列表
    vector<unique_ptr<BaseAST>>* array_index;
    // 初始化常量值
    unique_ptr<BaseAST> value;
    Result print() const override;
};

/**
 * @brief 常量初始化值 AST 类
 */
class ConstInitValAST : public BaseAST {
public:
    // 常量表达式
    optional<unique_ptr<BaseAST>> const_exp;
    // 初始化常量值列表，即 KoopaIR 中的 aggregate
    optional<vector<unique_ptr<BaseAST>>> init_values;
    // 初始化数组常量值
    void init(const vector<int>& indices, int*& arr, int& cur, int align);
    // 打印数组常量初始化值
    Result print(const string& ident, const vector<int>& indices);
    // 打印普通常量初始化值
    Result print() const override;
};

/**
 * @brief 常量表达式 AST 类
 */
class ConstExpAST : public BaseAST {
public:
    // 常量表达式
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

/**
 * @brief 变量声明 AST 类
 */
class VarDeclAST : public BaseAST {
public:
    // 变量定义列表
    vector<unique_ptr<BaseAST>> var_defs;
    Result print() const override;
};

/**
 * @brief 变量定义 AST 类
 */
class VarDefAST : public BaseAST {
public:
    // 变量名
    string ident;
    // 数组下标列表
    vector<unique_ptr<BaseAST>>* array_index;
    // 初始化变量值，可为空
    optional<unique_ptr<BaseAST>> value;
    Result print() const override;
};

/**
 * @brief 变量初始化值 AST 类
 */
class InitValAST : public BaseAST {
public:
    // 初始化表达式
    optional<unique_ptr<BaseAST>> exp;
    // 初始化值列表，即 KoopaIR 中的 aggregate
    optional<vector<unique_ptr<BaseAST>>> init_values;
    // 初始化数组变量值
    void init(const vector<int>& indices, int* arr, int& cur, int align);
    // 打印数组变量初始化值
    Result print(const string& ident, const vector<int>& indices);
    // 打印普通变量初始化值
    Result print() const override;
};

/**
 * @brief If 语句 AST 类
 */
class StmtIfAST : public BaseAST {
public:
    // 条件表达式
    unique_ptr<BaseAST> exp;
    // then 语句块
    unique_ptr<BaseAST> then_stmt;
    // else 语句块，可为空
    optional<unique_ptr<BaseAST>> else_stmt;
    Result print() const override;
};

/**
 * @brief While 语句 AST 类
 */
class StmtWhileAST : public BaseAST {
public:
    // 条件表达式
    unique_ptr<BaseAST> exp;
    // 循环体语句块
    unique_ptr<BaseAST> stmt;
    Result print() const override;
};

/**
 * @brief Break 语句 AST 类
 */
class StmtBreakAST : public BaseAST {
public:
    Result print() const override;
};

/**
 * @brief Continue 语句 AST 类
 */

class StmtContinueAST : public BaseAST {
public:
    Result print() const override;
};

/**
 * @brief 赋值语句 AST 类
 */
class StmtAssignAST : public BaseAST {
public:
    // 左值
    unique_ptr<BaseAST> l_val;
    // 右值
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

/**
 * @brief 表达式语句 AST 类
 */
class StmtExpAST : public BaseAST {
public:
    // 表达式，可为空，即单条 ';' 语句
    optional<unique_ptr<BaseAST>> exp;
    Result print() const override;
};

/**
 * @brief Return 语句 AST 类
 */
class StmtReturnAST : public BaseAST {
public:
    // 返回值，可为空
    optional<unique_ptr<BaseAST>> exp;
    Result print() const override;
};

/**
 * @brief 左值 AST 类
 */
class LValAST : public BaseAST {
public:
    // 变量名
    string ident;
    // 数组下标列表，可为空列表
    vector<unique_ptr<BaseAST>>* array_index;
    // 打印左值
    Result print() const override;
};

/**
 * @brief 表达式 AST 类
 */
class ExpAST : public BaseAST {
public:
    // 逻辑或表达式
    unique_ptr<BaseAST> l_or_exp;
    Result print() const override;
};

/**
 * @brief 逻辑或表达式 AST 类
 */
class LOrExpAST : public BaseAST {
public:
    // 逻辑与表达式
    unique_ptr<BaseAST> l_and_exp;
    Result print() const override;
};

/**
 * @brief 逻辑与表达式 AST 类
 */
class LAndExpAST : public BaseAST {
public:
    // 等值表达式
    unique_ptr<BaseAST> eq_exp;
    Result print() const override;
};

/**
 * @brief 逻辑与表达式 AST 类
 */
class LExpWithOpAST : public BaseAST {
public:
    // 逻辑运算符
    enum class LogicalOp {
        LOGICAL_OR,
        LOGICAL_AND
    };
    LogicalOp logical_op;
    // 左操作数
    unique_ptr<BaseAST> left;
    // 右操作数
    unique_ptr<BaseAST> right;
    Result print() const override;
};

/**
 * @brief 等值表达式 AST 类
 */
class EqExpAST : public BaseAST {
public:
    // 关系表达式
    unique_ptr<BaseAST> rel_exp;
    Result print() const override;
};

/**
 * @brief 等值表达式 AST 类
 */
class EqExpWithOpAST : public BaseAST {
public:
    // 等值运算符
    enum class EqOp {
        EQ,
        NEQ
    };
    EqOp eq_op;
    // 左操作数
    unique_ptr<BaseAST> left;
    // 右操作数
    unique_ptr<BaseAST> right;
    // 将字符串形式的运算符转换为等值运算符
    EqOp convert(const string& op) const;
    Result print() const override;
};

/**
 * @brief 关系表达式 AST 类
 */
class RelExpAST : public BaseAST {
public:
    // 加法表达式
    unique_ptr<BaseAST> add_exp;
    Result print() const override;
};

/**
 * @brief 关系表达式 AST 类
 */
class RelExpWithOpAST : public BaseAST {
public:
    // 关系运算符
    enum class RelOp {
        LE,
        GE,
        LT,
        GT
    };
    RelOp rel_op;
    // 左操作数
    unique_ptr<BaseAST> left;
    // 右操作数
    unique_ptr<BaseAST> right;
    // 将字符串形式的运算符转换为关系运算符
    RelOp convert(const string& op) const;
    Result print() const override;
};

/**
 * @brief 加法表达式 AST 类
 */
class AddExpAST : public BaseAST {
public:
    // 乘法表达式
    unique_ptr<BaseAST> mul_exp;
    Result print() const override;
};

/**
 * @brief 加法表达式 AST 类
 */
class AddExpWithOpAST : public BaseAST {
public:
    // 加法运算符
    enum class AddOp {
        ADD,
        SUB
    };
    AddOp add_op;
    // 左操作数
    unique_ptr<BaseAST> left;
    // 右操作数
    unique_ptr<BaseAST> right;
    // 将字符串形式的运算符转换为加法运算符
    AddOp convert(const string& op) const;
    Result print() const override;
};

/**
 * @brief 乘法表达式 AST 类
 */
class MulExpAST : public BaseAST {
public:
    // 一元表达式
    unique_ptr<BaseAST> unary_exp;
    Result print() const override;
};

/**
 * @brief 乘法表达式 AST 类
 */
class MulExpWithOpAST : public BaseAST {
public:
    // 乘法运算符
    enum class MulOp {
        MUL,
        DIV,
        MOD
    };
    MulOp mul_op;
    // 左操作数
    unique_ptr<BaseAST> left;
    // 右操作数
    unique_ptr<BaseAST> right;
    // 将字符串形式的运算符转换为乘法运算符
    MulOp convert(const string& op) const;
    Result print() const override;
};

/**
 * @brief 一元表达式 AST 类
 */
class UnaryExpAST : public BaseAST {
public:
    // 优先表达式
    unique_ptr<BaseAST> primary_exp;
    Result print() const override;
};

/**
 * @brief 带运算符的一元表达式 AST 类
 */
class UnaryExpWithOpAST : public BaseAST {
public:
    // 一元运算符
    enum class UnaryOp {
        POSITIVE,
        NEGATIVE,
        NOT
    };
    UnaryOp unary_op;
    // 一元表达式
    unique_ptr<BaseAST> unary_exp;
    // 将字符串形式的运算符转换为一元运算符
    UnaryOp convert(const string& op) const;
    Result print() const override;
};

/**
 * @brief 函数调用一元表达式 AST 类
 */
class UnaryExpWithFuncCallAST : public BaseAST {
public:
    // 函数名
    string ident;
    // 函数实参列表
    vector<unique_ptr<BaseAST>>* func_r_params;
    Result print() const override;
};

/**
 * @brief 优先表达式 AST 类
 */
class PrimaryExpAST : public BaseAST {
public:
    // 表达式
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

/**
 * @brief 数字字面量优先表达式 AST 类
 */
class PrimaryExpWithNumberAST : public BaseAST {
public:
    // 字面量
    int number;
    Result print() const override;
};

/**
 * @brief 左值优先表达式 AST 类
 */
class PrimaryExpWithLValAST : public BaseAST {
public:
    // 左值
    unique_ptr<BaseAST> l_val;
    Result print() const override;
};