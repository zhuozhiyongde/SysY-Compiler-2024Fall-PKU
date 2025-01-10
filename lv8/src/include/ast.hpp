#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <optional>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <typeinfo>
#include "include/frontend_utils.hpp"

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

class ProgramAST : public BaseAST {
public:
    vector<unique_ptr<BaseAST>> comp_units;
    Result print() const override;
};

class FuncDefAST : public BaseAST {
public:
    enum class FuncType {
        INT,
        VOID
    };
    FuncType func_type;
    string ident;
    vector<unique_ptr<BaseAST>>* func_f_params;
    unique_ptr<BaseAST> block;
    Result print() const override;
};

class FuncFParamAST : public BaseAST {
public:
    string ident;
    void as_param() const;
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
    bool is_global;
    Result print() const override;
};

class InitValAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

class StmtIfAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> then_stmt;
    optional<unique_ptr<BaseAST>> else_stmt;
    Result print() const override;
};

class StmtWhileAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> stmt;
    Result print() const override;
};

class StmtBreakAST : public BaseAST {
public:
    Result print() const override;
};

class StmtContinueAST : public BaseAST {
public:
    Result print() const override;
};

class StmtAssignAST : public BaseAST {
public:
    unique_ptr<BaseAST> l_val;
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

class StmtExpAST : public BaseAST {
public:
    optional<unique_ptr<BaseAST>> exp;
    Result print() const override;
};

class StmtReturnAST : public BaseAST {
public:
    optional<unique_ptr<BaseAST>> exp;
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

class UnaryExpWithFuncCallAST : public BaseAST {
public:
    string ident;
    vector<unique_ptr<BaseAST>>* func_r_params;
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