#pragma once
#include "koopa.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream> 
#include <unordered_map>
#include "include/other_utils.hpp"
#include "include/asm.hpp"

using namespace std;

void parse_riscv(const char* koopa_ir);

/**
 * @brief Riscv 类，用于生成 Riscv 汇编代码
 * @note - 包含 Riscv 汇编代码的生成方法
 * @note - 会自动处理偏置量，使之不超过 12 位限制
 */
class Riscv {
public:
    // 特殊语句
    void _data();
    void _text();
    void _globl(const string& name);
    void _word(const int& value);
    void _zero(const int& len);
    void _label(const string& name);
    // 调用与返回
    void _call(const string& ident);
    void _ret();
    // 单目运算
    void _seqz(const string& rd, const string& rs1);
    void _snez(const string& rd, const string& rs1);
    void _li(const string& rd, const int& imm);
    void _mv(const string& rd, const string& rs1);
    void _la(const string& rd, const string& rs1);
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
    void _sll(const string& rd, const string& rs1, const string& rs2);
    // 访存
    void _lw(const string& rd, const string& base, const int& bias);
    void _sw(const string& rs1, const string& base, const int& bias);
    // 分支
    void _bnez(const string& cond, const string& label);
    void _beqz(const string& cond, const string& label);
    void _jump(const string& label);
};

/**
 * @brief Context 类，用于管理栈空间
 * @note - stack_size：栈空间大小
 * @note - stack_used：栈空间已使用大小
 * @note - stack_map：栈空间映射，用于管理栈空间的使用情况
 */
class Context {
public:
    int stack_size;
    int stack_used = 0;
    bool save_ra = false;
    unordered_map<koopa_raw_value_t, int> stack_map;
    Context() : stack_size(0) {}
    Context(int stack_size) : stack_size(stack_size) {}
    void push(const koopa_raw_value_t& value, int bias);
};

/**
 * @brief ContextManager 类，用于管理 Context
 * @note - context_map：Context 映射，用于管理 Context 的使用情况
 */
class ContextManager {
private:
    int global_count = 0;
public:
    unordered_map<string, Context> context_map;
    unordered_map<koopa_raw_value_t, string> global_map;
    void create_context(const string& name, int stack_size);
    void create_global(const koopa_raw_value_t& value);
    Context& get_context(const string& name);
    string get_global(const koopa_raw_value_t& value);
};

/**
 * @brief RegisterManager 类，用于管理寄存器
 * @note - reg_count：寄存器计数器
 * @note - reg_map：寄存器映射，用于管理指令到寄存器的映射。
 */
class RegisterManager {
private:
    // 寄存器计数器
    int reg_count = 0;
public:
    unordered_map<koopa_raw_value_t, string> reg_map;
    string cur_reg();
    string new_reg();
    string tmp_reg();
    bool get_operand_reg(const koopa_raw_value_t& value);
    void reset();
};

extern ofstream riscv_ofs;
extern Riscv riscv;
extern Context context;
extern ContextManager context_manager;
extern RegisterManager register_manager;
