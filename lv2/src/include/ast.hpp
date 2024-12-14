#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

extern string mode;
extern ofstream debug_ofs;
extern ofstream koopa_ofs;
extern ofstream riscv_ofs;

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
    unique_ptr<BaseAST> func_def;
    void print() const override;
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;
    void print() const override;
};

class FuncTypeAST : public BaseAST {
public:
    string type;
    void print() const override;
};

class BlockAST : public BaseAST {
public:
    unique_ptr<BaseAST> stmt;
    void print() const override;
};

class StmtAST : public BaseAST {
public:
    int number;
    void print() const override;
};