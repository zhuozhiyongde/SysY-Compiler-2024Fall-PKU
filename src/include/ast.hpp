#pragma once

#include <memory>
#include <string>
#include <iostream>

extern std::string mode;

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void print() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    void print() const override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    void print() const override;
};

class FuncTypeAST : public BaseAST {
public:
    std::string type;
    void print() const override;
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;
    void print() const override;
};

class StmtAST : public BaseAST {
public:
    int number;
    void print() const override;
};