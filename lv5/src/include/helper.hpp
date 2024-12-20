#pragma once
#include "koopa.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream> 
#include <unordered_map>
#include "utils.hpp"

using namespace std;

extern ofstream riscv_ofs;

class Riscv {
public:
    // 控制转移类
    void _ret() {
        riscv_ofs << "\tret" << endl;
    }
    // 运算类
    // 单目运算
    void _seqz(const string& rd, const string& rs1) {
        riscv_ofs << "\tseqz " << rd << ", " << rs1 << endl;
    }
    void _snez(const string& rd, const string& rs1) {
        riscv_ofs << "\tsnez " << rd << ", " << rs1 << endl;
    }
    // 双目运算
    void _or(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\tor " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _and(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\tand " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _xor(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\txor " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _add(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\tadd " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _addi(const string& rd, const string& rs1, const int& imm) {
        riscv_ofs << "\taddi " << rd << ", " << rs1 << ", " << imm << endl;
    }
    void _sub(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\tsub " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _mul(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\tmul " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _div(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\tdiv " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _rem(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\trem " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _sgt(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\tsgt " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    void _slt(const string& rd, const string& rs1, const string& rs2) {
        riscv_ofs << "\tslt " << rd << ", " << rs1 << ", " << rs2 << endl;
    }
    // 加载和移动
    void _li(const string& rd, const int& imm) {
        riscv_ofs << "\tli " << rd << ", " << imm << endl;
    }
    void _mv(const string& rd, const string& rs1) {
        riscv_ofs << "\tmv " << rd << ", " << rs1 << endl;
    }
    void _lw(const string& rd, const string& rs1) {
        riscv_ofs << "\tlw " << rd << ", " << rs1 << endl;
    }
    void _sw(const string& rs1, const string& rs2) {
        riscv_ofs << "\tsw " << rs1 << ", " << rs2 << endl;
    }
};

extern Riscv riscv;

class Context {
public:
    int stack_size;
    int stack_used = 0;
    unordered_map<koopa_raw_value_t, string> stack_map;
    Context() : stack_size(0) {}
    Context(int stack_size) : stack_size(stack_size) {}
    void push(const koopa_raw_value_t& value, int bias) {
        assert(bias < stack_size);
        stack_map[value] = to_string(bias) + "(sp)";
    }
};

extern Context context;

class ContextManager {
public:
    unordered_map<string, Context> context_map;
    void create(const string& name, int stack_size) {
        context_map[name] = Context(stack_size);
    }
    Context& get(const string& name) {
        return context_map[name];
    }
};

class RegisterManager {
private:
    // 寄存器计数器
    int reg_count = 0;
public:
    // 存储指令到寄存器的映射
    unordered_map<koopa_raw_value_t, string> reg_map;
    string cur_reg() {
        // x0 是一个特殊的寄存器, 它的值恒为 0, 且向它写入的任何数据都会被丢弃.
        // t0 到 t6 寄存器, 以及 a0 到 a7 寄存器可以用来存放临时值.
        if (reg_count < 8) {
            return "t" + to_string(reg_count);
        }
        else {
            return "a" + to_string(reg_count - 8);
        }
    }
    string new_reg() {
        string reg = cur_reg();
        reg_count++;
        return reg;
    }
    // 返回是否需要使用寄存器，主要用于二元表达式存储结果
    // 注意！！这个函数是处理操作数的，而不是指令的
    bool get_operand_reg(const koopa_raw_value_t& value) {
        if (value->kind.tag == KOOPA_RVT_INTEGER) {
            if (value->kind.data.integer.value == 0) {
                reg_map[value] = "x0";
                return false;
            }
            else {
                reg_map[value] = new_reg();
                riscv._li(reg_map[value], value->kind.data.integer.value);
                return true;
            }
        }
        // 运算数为 load 指令，先加载
        else if (value->kind.tag == KOOPA_RVT_LOAD) {
            reg_map[value] = new_reg();
            riscv._lw(reg_map[value], context.stack_map[value]);
            return true;
        }
        // 运算数为二元运算的结果，也需要先加载
        // 出现在形如 a = a + b + c 的式子中
        else if (value->kind.tag == KOOPA_RVT_BINARY) {
            reg_map[value] = new_reg();
            riscv._lw(reg_map[value], context.stack_map[value]);
            return true;
        }
        return true;
    }
    void reset() {
        reg_count = 0;
    }
};
