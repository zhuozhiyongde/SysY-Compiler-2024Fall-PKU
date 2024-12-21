#pragma once
#include "koopa.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream> 
#include <unordered_map>
#include "include/other_utils.hpp"

using namespace std;

class Riscv {
public:
    void _ret();
    // 单目运算
    void _seqz(const string& rd, const string& rs1);
    void _snez(const string& rd, const string& rs1);
    void _li(const string& rd, const int& imm);
    void _mv(const string& rd, const string& rs1);
    // 双目运算
    void _or(const string& rd, const string& rs1, const string& rs2);
    void _and(const string& rd, const string& rs1, const string& rs2);
    void _xor(const string& rd, const string& rs1, const string& rs2);
    void _add(const string& rd, const string& rs1, const string& rs2);
    void _addi(const string& rd, const string& rs1, const int& imm);
    void _sub(const string& rd, const string& rs1, const string& rs2);
    void _mul(const string& rd, const string& rs1, const string& rs2);
    void _div(const string& rd, const string& rs1, const string& rs2);
    void _rem(const string& rd, const string& rs1, const string& rs2);
    void _sgt(const string& rd, const string& rs1, const string& rs2);
    void _slt(const string& rd, const string& rs1, const string& rs2);
    // 访存
    void _lw(const string& rd, const string& base, const int& bias);
    void _sw(const string& rs1, const string& base, const int& bias);
    // 栈操作
    void _add_sp(const int& bias);
    // 分支
    void _bnez(const string& cond, const string& label);
    void _beqz(const string& cond, const string& label);
    void _jump(const string& label);
};

class Context {
public:
    int stack_size;
    int stack_used = 0;
    unordered_map<koopa_raw_value_t, int> stack_map;
    Context() : stack_size(0) {}
    Context(int stack_size) : stack_size(stack_size) {}
    void push(const koopa_raw_value_t& value, int bias);
};

class ContextManager {
public:
    unordered_map<string, Context> context_map;
    void create(const string& name, int stack_size);
    Context& get(const string& name);
};

class RegisterManager {
private:
    // 寄存器计数器
    int reg_count = 0;
public:
    unordered_map<koopa_raw_value_t, string> reg_map;
    string cur_reg();
    string new_reg();
    bool get_operand_reg(const koopa_raw_value_t& value);
    void reset();
};

extern ofstream riscv_ofs;
extern Riscv riscv;
extern Context context;
extern ContextManager context_manager;
extern RegisterManager register_manager;
