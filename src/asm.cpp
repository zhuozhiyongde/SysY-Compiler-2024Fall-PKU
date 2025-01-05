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
    // 忽略库函数
    if (func->bbs.len == 0) {
        return;
    }
    // 访问所有基本块，bbs: basic block slice
    // 忽略函数名前的@，@main -> main
    riscv_ofs << endl;
    riscv_ofs << "\t.text" << endl;
    riscv_ofs << "\t.globl " << func->name + 1 << endl;
    riscv_ofs << func->name + 1 << ":" << endl;
    int cnt = 0;
    int stack_args = 0;
    bool has_call = false;
    for (size_t i = 0; i < func->bbs.len; ++i) {
        auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        cnt += bb->insts.len;
        for (size_t j = 0; j < bb->insts.len; ++j) {
            auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
            if (inst->ty->tag == KOOPA_RTT_UNIT) {
                cnt -= 1;
            }
            // 如果是 call 指令，需要额外计算变量表所需空间
            if (inst->kind.tag == KOOPA_RVT_CALL) {
                int args = inst->kind.data.call.args.len;
                // 取最大值，不同 call 之间可以覆写
                stack_args = max(stack_args, args - 8);
                has_call = true;
            }
        }
    }
    // 如果函数体内有 call 指令，需要多分配一条 store 指令来存储 ra 寄存器
    cnt += has_call;
    // 额外分配压栈参数空间
    cnt += stack_args;
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
    // 存储 ra 寄存器到 0(sp)
    if (has_call) {
        riscv._sw("ra", "sp", context.stack_size - 4);
        context.save_ra = true;
    }
    context.stack_used = stack_args * 4;
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
        // 由于需要在栈上记录返回结果，所以需要传入 value
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
    case KOOPA_RVT_CALL:
        // 访问 call 指令
        // 由于需要判断是否需要存储返回值，所以需要传入 value
        visit(kind.data.call, value);
        break;
    default:
        // 其他类型暂时遇不到
        cout << "Invalid instruction: " << koopaRawValueTagToString(kind.tag) << endl;
        assert(false);
    }
};

void visit(const koopa_raw_call_t& call, const koopa_raw_value_t& value) {
    int args = call.args.len;
    for (int i = 0; i < min(args, 8); i++) {
        // 这 8 个参数一定是存到 a0 - a7 寄存器中的
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        auto target = "a" + to_string(i);
        // 判断是否为整数
        if (arg->kind.tag == KOOPA_RVT_INTEGER) {
            riscv._li(target, arg->kind.data.integer.value);
        }
        // 为之前某操作的中间结果，实际上也必然为整数，但是需要先加载
        else {
            auto tmp = register_manager.new_reg();
            riscv._lw(tmp, "sp", context.stack_map[arg]);
            riscv._mv(target, tmp);
            register_manager.reset();
        }
    }
    // 处理超过 8 个参数的情况，此时需要将参数存到栈上
    for (int i = 8; i < args; i++) {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        int target = (i - 8) * 4;
        if (arg->kind.tag == KOOPA_RVT_INTEGER) {
            auto tmp = register_manager.new_reg();
            riscv._li(tmp, arg->kind.data.integer.value);
            riscv._sw(tmp, "sp", target);
            register_manager.reset();
        }
        // 为之前某操作的中间结果，实际上也必然为整数，但是需要先加载
        else {
            auto tmp = register_manager.new_reg();
            riscv._lw(tmp, "sp", context.stack_map[arg]);
            riscv._sw(tmp, "sp", target);
            register_manager.reset();
        }
    }
    // 调用函数
    riscv._call(call.callee->name + 1);
    // 判断是否需要存储返回值
    if (value->ty->tag != KOOPA_RTT_UNIT) {
        context.push(value, context.stack_used);
        riscv._sw("a0", "sp", context.stack_map[value]);
    }
}

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
    auto reg = register_manager.new_reg();
    auto bias = context.stack_used;
    riscv._lw(reg, "sp", context.stack_map[load.src]);
    context.push(value, bias);
    riscv._sw(reg, "sp", bias);
}

void visit(const koopa_raw_store_t& store) {
    register_manager.get_operand_reg(store.value);
    // 如果 dest 不在 stack_map 中，则需要分配新的空间
    if (context.stack_map.find(store.dest) == context.stack_map.end()) {
        context.push(store.dest, context.stack_used);
    }
    assert(register_manager.reg_map[store.value] != "");
    riscv._sw(register_manager.reg_map[store.value], "sp", context.stack_map[store.dest]);
}

void visit(const koopa_raw_return_t& ret) {
    // 先把值搞到 a0 寄存器中
    if (ret.value != nullptr) {
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
        else if (ret.value->kind.tag == KOOPA_RVT_CALL) {
            // 此时必然是有返回值的
            riscv._lw("a0", "sp", context.stack_map[ret.value]);
        }
        else {
            riscv._li("a0", 0);
        }
    }
    // 返回
    if (context.save_ra) {
        riscv._lw("ra", "sp", context.stack_size - 4);
    }
    // 恢复栈指针
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
    context.push(value, context.stack_used);
    riscv._sw(cur, "sp", context.stack_map[value]);
}
