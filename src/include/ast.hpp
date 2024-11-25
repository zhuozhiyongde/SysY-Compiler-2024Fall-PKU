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

// 在使用前需要先声明
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
    unique_ptr<BaseAST> add_exp;
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
    Result print() const override;
};