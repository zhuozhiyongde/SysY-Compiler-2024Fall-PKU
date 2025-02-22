#include "include/asm.hpp"

// Riscv 辅助类，用于生成 riscv 汇编代码
Riscv riscv;
// 全局 context 管理器
ContextManager context_manager;
// 当前函数对应的 context
Context context;
// 寄存器管理器
RegisterManager register_manager;

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
    int cnt = 0;
    for (size_t i = 0; i < func->bbs.len; ++i) {
        auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        cnt += bb->insts.len;
        for (size_t j = 0; j < bb->insts.len; ++j) {
            auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
            if (inst->ty->tag == KOOPA_RTT_UNIT) {
                cnt -= 1;
            }
        }
    }
    cnt *= 4;
    // 对齐到 16 的倍数
    cnt = (cnt + 15) / 16 * 16;
    // 检查是否超过 imm12 的限制
    // cnt = 8000;
    context_manager.create(func->name + 1, cnt);
    context = context_manager.get(func->name + 1);
    riscv._addi("sp", "sp", -cnt);
    // 检查是否超过 imm12 的限制
    // context.stack_used = 2040;
    visit(func->bbs);
};
void visit(const koopa_raw_basic_block_t& bb) {
    // 访问所有指令，insts: instruction slice
    riscv_ofs << bb->name + 1 << ":" << endl;
    visit(bb->insts);
};
void visit(const koopa_raw_value_t& value) {
    // 根据指令类型判断后续需要如何访问
    const auto& kind = value->kind;
    // RVT: Raw Value Tag, 区分指令类型
    register_manager.reset();
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
    case KOOPA_RVT_ALLOC:
        // 访问 alloc 指令，不管
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        visit(kind.data.store);
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        visit(kind.data.load, value);
        break;
    case KOOPA_RVT_BRANCH:
        // 访问 branch 指令
        visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        // 访问 jump 指令
        visit(kind.data.jump);
        break;
    default:
        // 其他类型暂时遇不到
        cout << "Invalid instruction: " << kind.tag << endl;
        assert(false);
    }
};

void visit(const koopa_raw_branch_t& branch) {
    // 访问 branch 指令
    register_manager.get_operand_reg(branch.cond);
    auto cond = register_manager.reg_map[branch.cond];
    riscv._bnez(cond, branch.true_bb->name + 1);
    riscv._beqz(cond, branch.false_bb->name + 1);
}

void visit(const koopa_raw_jump_t& jump) {
    riscv._jump(jump.target->name + 1);
}

void visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value) {
    // 取一个临时的寄存器，不修改 reg_count
    auto reg = register_manager.new_reg();
    auto bias = context.stack_used;
    riscv._lw(reg, "sp", context.stack_map[load.src]);
    context.push(value, bias);
    context.stack_used += 4;
    riscv._sw(reg, "sp", bias);
}

void visit(const koopa_raw_store_t& store) {
    register_manager.get_operand_reg(store.value);
    // 如果 dest 不在 stack_map 中，则需要分配新的空间
    if (context.stack_map.find(store.dest) == context.stack_map.end()) {
        context.stack_map[store.dest] = context.stack_used;
        context.stack_used += 4;
    }
    riscv._sw(register_manager.reg_map[store.value], "sp", context.stack_map[store.dest]);
}

void visit(const koopa_raw_return_t& ret) {
    // 形如 ret 1 直接返回整数的
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
        riscv._li("a0", ret.value->kind.data.integer.value);
    }
    // 形如 ret exp, 返回表达式的值
    else if (ret.value->kind.tag == KOOPA_RVT_BINARY) {
        riscv._lw("a0", "sp", context.stack_map[ret.value]);
        // 或者使用 mv 指令，将寄存器中的值赋值给 a0
        // riscv._mv("a0", register_manager.reg_map[ret.value]);
    }
    else if (ret.value->kind.tag == KOOPA_RVT_LOAD) {
        riscv._lw("a0", "sp", context.stack_map[ret.value]);
    }
    else {
        riscv._li("a0", 0);
    }
    riscv._addi("sp", "sp", context.stack_size);
    riscv._ret();
};
void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
    // 访问 binary 指令
    bool lhs_use_reg = register_manager.get_operand_reg(binary.lhs);
    bool rhs_use_reg = register_manager.get_operand_reg(binary.rhs);

    // 确定中间结果的寄存器
    // 如果两个都是整数，显然要新开一个寄存器，来存储中间结果
    if (!lhs_use_reg && !rhs_use_reg) {
        register_manager.reg_map[value] = register_manager.new_reg();
    }
    // 对于其他情况，找一个已有寄存器来存储中间结果
    else if (lhs_use_reg) {
        register_manager.reg_map[value] = register_manager.reg_map[binary.lhs];
    }
    else {
        register_manager.reg_map[value] = register_manager.reg_map[binary.rhs];
    }

    const auto cur = register_manager.reg_map[value];
    const auto lhs = register_manager.reg_map[binary.lhs];
    const auto rhs = register_manager.reg_map[binary.rhs];

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
    // 把结果存回栈中
    context.stack_map[value] = context.stack_used;
    riscv._sw(cur, "sp", context.stack_map[value]);
    context.stack_used += 4;
}
