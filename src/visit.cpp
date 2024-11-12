#include "include/visit.hpp"

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
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        visit(kind.data.integer);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
};
void visit(const koopa_raw_return_t& ret) {
    if (ret.value) {
        riscv_ofs << "\tli a0, ";
        visit(ret.value->kind.data.integer);
        riscv_ofs << endl;
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


