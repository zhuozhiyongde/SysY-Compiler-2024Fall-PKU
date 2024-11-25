#pragma once
#include "koopa.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream> 

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
};


