#include "include/backend_utils.hpp"

void Riscv::_ret() {
    riscv_ofs << "\tret" << endl;
}

void Riscv::_seqz(const string& rd, const string& rs1) {
    riscv_ofs << "\tseqz " << rd << ", " << rs1 << endl;
}

void Riscv::_snez(const string& rd, const string& rs1) {
    riscv_ofs << "\tsnez " << rd << ", " << rs1 << endl;
}

void Riscv::_or(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tor " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_and(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tand " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_xor(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\txor " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_add(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tadd " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_addi(const string& rd, const string& rs1, const int& imm) {
    riscv_ofs << "\taddi " << rd << ", " << rs1 << ", " << imm << endl;
}

void Riscv::_sub(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tsub " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_mul(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tmul " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_div(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tdiv " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_rem(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\trem " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_sgt(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tsgt " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_slt(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tslt " << rd << ", " << rs1 << ", " << rs2 << endl;
}

void Riscv::_li(const string& rd, const int& imm) {
    riscv_ofs << "\tli " << rd << ", " << imm << endl;
}

void Riscv::_mv(const string& rd, const string& rs1) {
    riscv_ofs << "\tmv " << rd << ", " << rs1 << endl;
}

void Riscv::_lw(const string& rd, const string& base, const int& bias) {
    // 检查偏移量是否在 12 位立即数范围内
    if (bias >= -2048 && bias < 2048) {
        riscv_ofs << "\tlw " << rd << ", " << bias << "(" << base << ")" << endl;
    }
    else {
        auto reg = register_manager.new_reg();
        _li(reg, bias);
        riscv_ofs << "\tlw " << rd << ", " << reg << "(" << base << ")" << endl;
    }
}

void Riscv::_sw(const string& rs1, const string& base, const int& bias) {
    // 检查偏移量是否在 12 位立即数范围内
    if (bias >= -2048 && bias < 2048) {
        riscv_ofs << "\tsw " << rs1 << ", " << bias << "(" << base << ")" << endl;
    }
    else {
        auto reg = register_manager.new_reg();
        _li(reg, bias);
        riscv_ofs << "\tsw " << rs1 << ", " << reg << "(" << base << ")" << endl;
    }
}

void Riscv::_add_sp(const int& bias) {
    if (bias >= -2048 && bias < 2048) {
        riscv_ofs << "\taddi sp, sp, " << bias << endl;
    }
    else {
        auto reg = register_manager.new_reg();
        _li(reg, bias);
        riscv_ofs << "\tadd sp, sp, " << reg << endl;
    }
}

void Riscv::_bnez(const string& cond, const string& label) {
    riscv_ofs << "\tbnez " << cond << ", " << label << endl;
}

void Riscv::_beqz(const string& cond, const string& label) {
    riscv_ofs << "\tbeqz " << cond << ", " << label << endl;
}

void Riscv::_jump(const string& label) {
    riscv_ofs << "\tj " << label << endl;
}

void Context::push(const koopa_raw_value_t& value, int bias) {
    assert(bias < stack_size);
    stack_map[value] = bias;
}


void ContextManager::create(const string& name, int stack_size) {
    context_map[name] = Context(stack_size);
}

Context& ContextManager::get(const string& name) {
    return context_map[name];
}

string RegisterManager::cur_reg() {
    // x0 是一个特殊的寄存器, 它的值恒为 0, 且向它写入的任何数据都会被丢弃.
    // t0 到 t6 寄存器, 以及 a0 到 a7 寄存器可以用来存放临时值.
    if (reg_count < 8) {
        return "t" + to_string(reg_count);
    }
    else {
        return "a" + to_string(reg_count - 8);
    }
}

string RegisterManager::new_reg() {
    string reg = cur_reg();
    reg_count++;
    return reg;
}

bool RegisterManager::get_operand_reg(const koopa_raw_value_t& value) {
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
        riscv._lw(reg_map[value], "sp", context.stack_map[value]);
        return true;
    }
    // 运算数为二元运算的结果，也需要先加载
    // 出现在形如 a = a + b + c 的式子中
    else if (value->kind.tag == KOOPA_RVT_BINARY) {
        reg_map[value] = new_reg();
        riscv._lw(reg_map[value], "sp", context.stack_map[value]);
        return true;
    }
    return true;
}

void RegisterManager::reset() {
    reg_count = 0;
}
