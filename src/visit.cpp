#include "include/visit.hpp"
#include "include/utils.hpp"
#include <unordered_map>

// 寄存器计数器
int reg_count = 0;
// 符号到寄存器的映射，对二元表达式，可以以 lhs 或 rhs 为 key，获得其结果的寄存器
unordered_map<koopa_raw_value_t, string> symbol_map;

string get_reg() {
    // x0 是一个特殊的寄存器, 它的值恒为 0, 且向它写入的任何数据都会被丢弃.
    // t0 到 t6 寄存器, 以及 a0 到 a7 寄存器可以用来存放临时值.
    if (reg_count < 8) {
        return "t" + to_string(reg_count);
    }
    else {
        return "a" + to_string(reg_count - 8);
    }
}

bool is_use_reg(const koopa_raw_value_t& value) {
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
        if (value->kind.data.integer.value == 0) {
            symbol_map[value] = "x0";
        }
        else {
            auto reg = get_reg();
            // 加载指令：load immediate
            riscv_ofs << "\tli " << reg << ", ";
            visit(value->kind.data.integer);
            riscv_ofs << endl;
            symbol_map[value] = reg;
            reg_count++;
        }
        return true;
    }
    return false;
}

void visit(const koopa_raw_program_t& program) {
    // 访问所有全局变量
    visit(program.values);
    // 访问所有函数
    riscv_ofs << "\t.text" << endl;
    visit(program.funcs);
};
void visit(const koopa_raw_slice_t& slice) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        // RSIK: Raw Slice Kind, 区分 slice 的类型
        switch (slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            cout << slice.kind << endl;
            assert(false);
        }
    }
};
void visit(const koopa_raw_function_t& func) {
    // 访问所有基本块，bbs: basic block slice
    // 忽略函数名前的@，@main -> main
    riscv_ofs << "\t.globl " << func->name + 1 << endl;
    riscv_ofs << func->name + 1 << ":" << endl;
    visit(func->bbs);
};
void visit(const koopa_raw_basic_block_t& bb) {
    // 访问所有指令，insts: instruction slice
    visit(bb->insts);
};
void visit(const koopa_raw_value_t& value) {
    // 根据指令类型判断后续需要如何访问
    const auto& kind = value->kind;
    // RVT: Raw Value Tag, 区分指令类型
    switch (kind.tag) {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        // Koopa：
        // ret %2
        // ret 2
        visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        // 不是形如 %1 = sub 0, %0 这种指令
        // 结构：
        // ├── op: koopa_raw_binary_op_t
        // ├── lhs: koopa_raw_value_t
        // └── rhs: koopa_raw_value_t
        visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        visit(kind.data.binary, value);
        break;
    default:
        // 其他类型暂时遇不到
        cout << "Invalid instruction: " << kind.tag << endl;
        assert(false);
    }
};
void visit(const koopa_raw_return_t& ret) {
    // 形如 ret 1 直接返回整数的
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_ofs << "\tli a0, ";
        visit(ret.value->kind.data.integer);
        riscv_ofs << endl;
        riscv_ofs << "\tret" << endl;
    }
    // 形如 ret exp, 返回表达式的值
    else if (ret.value->kind.tag == KOOPA_RVT_BINARY) {
        riscv_ofs << "\tmv a0, " << symbol_map[ret.value] << endl;
        riscv_ofs << "\tret" << endl;
    }
    else {
        riscv_ofs << "\tli a0, 0" << endl;
        riscv_ofs << "\tret" << endl;
    }
};
void visit(const koopa_raw_integer_t& integer) {
    riscv_ofs << integer.value;
};

unordered_map<koopa_raw_binary_op_t, string> binary_op_map = {
    {KOOPA_RBO_EQ, "xor"},
};

void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
    // 访问 binary 指令
    bool lhs_use_reg = is_use_reg(binary.lhs);
    bool rhs_use_reg = is_use_reg(binary.rhs);

    symbol_map[value] = get_reg();
    reg_count++;
    switch (binary.op) {
    case KOOPA_RBO_EQ:
        riscv_ofs << "\txor " << symbol_map[value] << ", " << symbol_map[binary.lhs] << ", " << symbol_map[binary.rhs] << endl;
        riscv_ofs << "\tseqz " << symbol_map[value] << ", " << symbol_map[value] << endl;
        break;
    case KOOPA_RBO_SUB:
        riscv_ofs << "\tsub " << symbol_map[value] << ", " << symbol_map[binary.lhs] << ", " << symbol_map[binary.rhs] << endl;
        break;
    case KOOPA_RBO_ADD:
        riscv_ofs << "\tadd " << symbol_map[value] << ", " << symbol_map[binary.lhs] << ", " << symbol_map[binary.rhs] << endl;
        break;
    case KOOPA_RBO_MUL:
        riscv_ofs << "\tmul " << symbol_map[value] << ", " << symbol_map[binary.lhs] << ", " << symbol_map[binary.rhs] << endl;
        break;
    case KOOPA_RBO_DIV:
        riscv_ofs << "\tdiv " << symbol_map[value] << ", " << symbol_map[binary.lhs] << ", " << symbol_map[binary.rhs] << endl;
        break;
    case KOOPA_RBO_MOD:
        riscv_ofs << "\trem " << symbol_map[value] << ", " << symbol_map[binary.lhs] << ", " << symbol_map[binary.rhs] << endl;
        break;
    default:
        riscv_ofs << "Invalid binary operation: " << koopaRawBinaryOpToString(binary.op) << endl;
    }
}
