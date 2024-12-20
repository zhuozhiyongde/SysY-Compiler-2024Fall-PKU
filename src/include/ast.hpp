#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <optional>
#include <vector>
#include <unordered_map>
#include <cassert>
#include "include/type.hpp"

using namespace std;

extern string mode;
extern ofstream debug_ofs;
extern ofstream koopa_ofs;
extern ofstream riscv_ofs;

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual Result print() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    unique_ptr<BaseAST> func_def;
    Result print() const override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;
    Result print() const override;
};

class FuncTypeAST : public BaseAST {
public:
    string type;
    Result print() const override;
};

class BlockAST : public BaseAST {
public:
    vector<unique_ptr<BaseAST>> block_items;
    Result print() const override;
};

class ConstDeclAST : public BaseAST {
public:
    vector<unique_ptr<BaseAST>> const_defs;
    Result print() const override;
};

class ConstDefAST : public BaseAST {
public:
    string ident;
    unique_ptr<BaseAST> value;
    Result print() const override;
};

class ConstInitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> const_exp;
    Result print() const override;
};

class ConstExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

class VarDeclAST : public BaseAST {
public:
    vector<unique_ptr<BaseAST>> var_defs;
    Result print() const override;
};

class VarDefAST : public BaseAST {
public:
    string ident;
    optional<unique_ptr<BaseAST>> value;
    Result print() const override;
};

class InitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

class StmtAssignAST : public BaseAST {
public:
    unique_ptr<BaseAST> l_val;
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

class StmtReturnAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

class LValAST : public BaseAST {
public:
    string ident;
    Result print() const override;
};

class ExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> l_or_exp;
    Result print() const override;
};

class LOrExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> l_and_exp;
    Result print() const override;
};

class LAndExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> eq_exp;
    Result print() const override;
};

class LExpWithOpAST : public BaseAST {
public:
    enum class LogicalOp {
        LOGICAL_OR,
        LOGICAL_AND
    };
    LogicalOp logical_op;
    unique_ptr<BaseAST> left;
    unique_ptr<BaseAST> right;
    Result print() const override;
};

class EqExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> rel_exp;
    Result print() const override;
};

class EqExpWithOpAST : public BaseAST {
public:
    enum class EqOp {
        EQ,
        NEQ
    };
    EqOp eq_op;
    unique_ptr<BaseAST> left;
    unique_ptr<BaseAST> right;
    EqOp convert(const string& op) const;
    Result print() const override;
};

class RelExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> add_exp;
    Result print() const override;
};

class RelExpWithOpAST : public BaseAST {
public:
    enum class RelOp {
        LE,
        GE,
        LT,
        GT
    };
    RelOp rel_op;
    unique_ptr<BaseAST> left;
    unique_ptr<BaseAST> right;
    RelOp convert(const string& op) const;
    Result print() const override;
};

class AddExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> mul_exp;
    Result print() const override;
};

class AddExpWithOpAST : public BaseAST {
public:
    enum class AddOp {
        ADD,
        SUB
    };
    AddOp add_op;
    unique_ptr<BaseAST> left;
    unique_ptr<BaseAST> right;
    AddOp convert(const string& op) const;
    Result print() const override;
};

class MulExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> unary_exp;
    Result print() const override;
};

class MulExpWithOpAST : public BaseAST {
public:
    enum class MulOp {
        MUL,
        DIV,
        MOD
    };
    MulOp mul_op;
    unique_ptr<BaseAST> left;
    unique_ptr<BaseAST> right;
    MulOp convert(const string& op) const;
    Result print() const override;
};

class UnaryExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> primary_exp;
    Result print() const override;
};


class UnaryExpWithOpAST : public BaseAST {
public:
    enum class UnaryOp {
        POSITIVE,
        NEGATIVE,
        NOT
    };
    UnaryOp unary_op;
    unique_ptr<BaseAST> unary_exp;
    UnaryOp convert(const string& op) const;
    Result print() const override;
};

class PrimaryExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

class PrimaryExpWithNumberAST : public BaseAST {
public:
    int number;
    Result print() const override;
};

class PrimaryExpWithLValAST : public BaseAST {
public:
    unique_ptr<BaseAST> l_val;
    Result print() const override;
};