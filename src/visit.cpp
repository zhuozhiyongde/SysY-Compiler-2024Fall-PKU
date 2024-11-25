#include "include/visit.hpp"

// 寄存器计数器
int reg_count = 0;
// 符号到寄存器的映射，对二元表达式，可以以 lhs 或 rhs 为 key，获得其结果的寄存器
unordered_map<koopa_raw_value_t, string> symbol_map;

Riscv riscv;

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
            // 只有 0 的话，不需要占用新的寄存器
            return false;
        }
        else {
            // 说明是个立即数，需要加载到寄存器中
            auto reg = get_reg();
            riscv._li(reg, value->kind.data.integer.value);
            symbol_map[value] = reg;
            reg_count++;
            // 由于存在一个中间结果，会占用一个寄存器
            return true;
        }
    }
    // 对于中间结果，会占用一个寄存器
    return true;
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
        visit(kind.data.ret);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令（双目运算）
        // 由于要存储单条指令的计算结果，所以将指令本身 value 也传入
        // 用于后续在 symbol_map 中存储结果
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
        riscv._li("a0", ret.value->kind.data.integer.value);
        riscv._ret();
    }
    // 形如 ret exp, 返回表达式的值
    else if (ret.value->kind.tag == KOOPA_RVT_BINARY) {
        riscv._mv("a0", symbol_map[ret.value]);
        riscv._ret();
    }
    else {
        riscv._li("a0", 0);
        riscv._ret();
    }
};
void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
    // 访问 binary 指令
    bool lhs_use_reg = is_use_reg(binary.lhs);
    bool rhs_use_reg = is_use_reg(binary.rhs);

    // 确定中间结果的寄存器
    // 如果两个都是整数，显然要新开一个寄存器，来存储中间结果
    if (!lhs_use_reg && !rhs_use_reg) {
        symbol_map[value] = get_reg();
        reg_count++;
    }
    // 对于其他情况，找一个已有寄存器来存储中间结果
    else if (lhs_use_reg) {
        symbol_map[value] = symbol_map[binary.lhs];
    }
    else {
        symbol_map[value] = symbol_map[binary.rhs];
    }

    const auto cur = symbol_map[value];
    const auto lhs = symbol_map[binary.lhs];
    const auto rhs = symbol_map[binary.rhs];

    switch (binary.op) {
    case KOOPA_RBO_EQ:
        riscv._xor(cur, lhs, rhs);
        riscv._seqz(cur, cur);
        break;
    case KOOPA_RBO_NOT_EQ:
        riscv._xor(cur, lhs, rhs);
        riscv._snez(cur, cur);
        break;
    case KOOPA_RBO_LE:
        // lhs <= rhs 等价于 !(lhs > rhs)
        riscv._sgt(cur, lhs, rhs);
        riscv._seqz(cur, cur);
        break;
    case KOOPA_RBO_GE:
        // lhs >= rhs 等价于 !(lhs < rhs)
        riscv._slt(cur, lhs, rhs);
        riscv._seqz(cur, cur);
        break;
    case KOOPA_RBO_LT:
        riscv._slt(cur, lhs, rhs);
        break;
    case KOOPA_RBO_GT:
        riscv._sgt(cur, lhs, rhs);
        break;
    case KOOPA_RBO_OR:
        riscv._or(cur, lhs, rhs);
        break;
    case KOOPA_RBO_AND:
        riscv._and(cur, lhs, rhs);
        break;
    case KOOPA_RBO_SUB:
        riscv._sub(cur, lhs, rhs);
        break;
    case KOOPA_RBO_ADD:
        riscv._add(cur, lhs, rhs);
        break;
    case KOOPA_RBO_MUL:
        riscv._mul(cur, lhs, rhs);
        break;
    case KOOPA_RBO_DIV:
        riscv._div(cur, lhs, rhs);
        break;
    case KOOPA_RBO_MOD:
        riscv._rem(cur, lhs, rhs);
        break;
    default:
        riscv_ofs << "Invalid binary operation: " << koopaRawBinaryOpToString(binary.op) << endl;
    }
}
