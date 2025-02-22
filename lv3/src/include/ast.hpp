#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <optional>

using namespace std;

extern string mode;
extern ofstream debug_ofs;
extern ofstream koopa_ofs;
extern ofstream riscv_ofs;

// 在使用前需要先声明（这里是有一次想要强类型尝试 unique_ptr 导致的，实际上暂时不需要）
class CompUnitAST;
class FuncDefAST;
class FuncTypeAST;
class BlockAST;
class StmtAST;
class ExpAST;
class AddExpAST;
class AddExpWithOpAST;
class MulExpAST;
class MulExpWithOpAST;
class UnaryExpAST;
class PrimaryExpAST;
class UnaryExpWithOpAST;


class Result {
public:
    enum class Type {
        IMM,
        REG
    };
    Type type;
    int val;
    friend ostream& operator<<(ostream& os, const Result& result) {
        os << (result.type == Type::REG ? "%" : "") << result.val;
        return os;
    }
    Result() : type(Type::IMM), val(0) {}
    Result(Type type, int val) : type(type), val(val) {}
};

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
    unique_ptr<BaseAST> stmt;
    Result print() const override;
};

class StmtAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    Result print() const override;
};

class ExpAST : public BaseAST {
public:
    unique_ptr<BaseAST> l_or_exp;
    Result print() const override;
};

class LOrExpAST : public BaseAST {
public:
    optional<unique_ptr<BaseAST>> l_and_exp;
    optional<unique_ptr<BaseAST>> l_exp_with_op;
    Result print() const override;
};


class LAndExpAST : public BaseAST {
public:
    optional<unique_ptr<BaseAST>> eq_exp;
    optional<unique_ptr<BaseAST>> l_exp_with_op;
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
    optional<unique_ptr<BaseAST>> rel_exp;
    optional<unique_ptr<BaseAST>> eq_exp_with_op;
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
    optional<unique_ptr<BaseAST>> add_exp;
    optional<unique_ptr<BaseAST>> rel_exp_with_op;
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
    optional<unique_ptr<BaseAST>> mul_exp;
    optional<unique_ptr<BaseAST>> add_exp_with_op;
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
    optional<unique_ptr<BaseAST>> unary_exp;
    optional<unique_ptr<BaseAST>> mul_exp_with_op;
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
    optional<unique_ptr<BaseAST>> primary_exp;
    optional<unique_ptr<BaseAST>> unary_exp_with_op;
    Result print() const override;
};

class PrimaryExpAST : public BaseAST {
public:
    optional<unique_ptr<BaseAST>> exp;
    optional<int> number;
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