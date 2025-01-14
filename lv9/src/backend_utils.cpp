#include "include/backend_utils.hpp"

/**
 * @brief 解析 KoopaIR 字符串，生成 Riscv 汇编代码
 * @param[in] koopa_ir KoopaIR 字符串
 */
void parse_riscv(const char* koopa_ir) {
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(koopa_ir, &program);
    // 确保解析时没有出错
    assert(ret == KOOPA_EC_SUCCESS);
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    visit(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}

/**
 * @brief 判断一个数是否为 2 的幂次
 * @param[in] x 要判断的数
 * @return 是否为 2 的幂次，若为 2 的幂次则返回幂次，否则返回 -1
 */
int is_power_of_two(int x) {
    if (x <= 0) {
        return -1;
    }
    double logResult = log2(x);
    if (floor(logResult) == logResult) {
        return static_cast<int>(logResult);
    }
    else {
        return -1;
    }
}

/**
 * @brief 生成 .data 宏
 */
void Riscv::_data() {
    riscv_ofs << "\t.data" << endl;
}

/**
 * @brief 生成 .text 宏
 */
void Riscv::_text() {
    riscv_ofs << "\t.text" << endl;
}

/**
 * @brief 生成 .globl name 宏
 * @param[in] name 全局变量名
 */
void Riscv::_globl(const string& name) {
    riscv_ofs << "\t.globl " << name << endl;
}

/**
 * @brief 生成 .word value 宏
 * @param[in] value 要存储的值
 */
void Riscv::_word(const int& value) {
    riscv_ofs << "\t.word " << value << endl;
}

/**
 * @brief 生成 .zero len 宏
 * @param[in] len 要填充的 0 的个数
 */
void Riscv::_zero(const int& len) {
    riscv_ofs << "\t.zero " << len << endl;
}

/**
 * @brief 生成 label 标签，即 `name:`
 * @param[in] name 标签名
 */
void Riscv::_label(const string& name) {
    riscv_ofs << name << ":" << endl;
}

/**
 * @brief 生成 call ident 指令
 * @param[in] ident 函数名
 */
void Riscv::_call(const string& ident) {
    riscv_ofs << "\tcall " << ident << endl;
}

/**
 * @brief 生成 ret 指令
 */
void Riscv::_ret() {
    riscv_ofs << "\tret" << endl;
}

/**
 * @brief 生成 seqz 指令，即比较 rs1 是否为 0，若为 0 则将 rd 置 1，否则置 0
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器
 */
void Riscv::_seqz(const string& rd, const string& rs1) {
    riscv_ofs << "\tseqz " << rd << ", " << rs1 << endl;
}

/**
 * @brief 生成 snez 指令，即比较 rs1 是否为 0，若不为 0 则将 rd 置 1，否则置 0
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器
 */
void Riscv::_snez(const string& rd, const string& rs1) {
    riscv_ofs << "\tsnez " << rd << ", " << rs1 << endl;
}

/**
 * @brief 生成 or 指令，即 rd = rs1 | rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_or(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tor " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 and 指令，即 rd = rs1 & rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_and(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tand " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 xor 指令，即 rd = rs1 ^ rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_xor(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\txor " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 add 指令，即 rd = rs1 + rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_add(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tadd " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 addi 指令，即 rd = rs1 + imm，会自动处理 12 位立即数限制
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器
 * @param[in] imm 立即数
 * @note 如果 imm 超过 12 位立即数限制，则会先将其存入一个临时寄存器，再进行加法运算
 */
void Riscv::_addi(const string& rd, const string& rs1, const int& imm) {
    if (imm >= -2048 && imm < 2048) {
        riscv_ofs << "\taddi " << rd << ", " << rs1 << ", " << imm << endl;
    }
    else {
        auto reg = register_manager.tmp_reg();
        _li(reg, imm);
        _add(rd, rs1, reg);
    }
}

/**
 * @brief 生成 sub 指令，即 rd = rs1 - rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_sub(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tsub " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 mul 指令，即 rd = rs1 * rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_mul(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tmul " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 div 指令，即 rd = rs1 / rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_div(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tdiv " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 rem（取余）指令，即 rd = rs1 % rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_rem(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\trem " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 sgt 指令，即 rd = rs1 > rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_sgt(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tsgt " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 slt 指令，即 rd = rs1 < rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_slt(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tslt " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 sll（左移）指令，即 rd = rs1 << rs2
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器 1
 * @param[in] rs2 源寄存器 2
 */
void Riscv::_sll(const string& rd, const string& rs1, const string& rs2) {
    riscv_ofs << "\tsll " << rd << ", " << rs1 << ", " << rs2 << endl;
}

/**
 * @brief 生成 li（加载立即数）指令，即 rd = imm
 * @param[in] rd 目标寄存器
 * @param[in] imm 立即数
 */
void Riscv::_li(const string& rd, const int& imm) {
    riscv_ofs << "\tli " << rd << ", " << imm << endl;
}

/**
 * @brief 生成 mv（移动）指令，即 rd = rs1
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器
 */
void Riscv::_mv(const string& rd, const string& rs1) {
    riscv_ofs << "\tmv " << rd << ", " << rs1 << endl;
}

/**
 * @brief 生成 la（加载地址）指令，即 rd = rs1
 * @param[in] rd 目标寄存器
 * @param[in] rs1 源寄存器
 */
void Riscv::_la(const string& rd, const string& rs1) {
    riscv_ofs << "\tla " << rd << ", " << rs1 << endl;
}

/**
 * @brief 生成 lw（加载字）指令，即 rd = *(base + bias)
 * @param[in] rd 目标寄存器
 * @param[in] base 基址寄存器
 * @param[in] bias 偏移量
 * @note 会自动处理偏移量，若偏移量超过 12 位立即数限制，则先将其存入一个临时寄存器，再进行加法运算
 */
void Riscv::_lw(const string& rd, const string& base, const int& bias) {
    // 检查偏移量是否在 12 位立即数范围内
    if (bias >= -2048 && bias < 2048) {
        riscv_ofs << "\tlw " << rd << ", " << bias << "(" << base << ")" << endl;
    }
    else {
        auto reg = register_manager.tmp_reg();
        _li(reg, bias);
        _add(reg, base, reg);
        riscv_ofs << "\tlw " << rd << ", " << "(" << reg << ")" << endl;
    }
}

/**
 * @brief 生成 sw（存储字）指令，即 *(base + bias) = rs1
 * @param[in] rs 源寄存器
 * @param[in] base 基址寄存器
 * @param[in] bias 偏移量
 * @note 会自动处理偏移量，若偏移量超过 12 位立即数限制，则先将其存入一个临时寄存器，再进行加法运算
 */
void Riscv::_sw(const string& rs, const string& base, const int& bias) {
    // 检查偏移量是否在 12 位立即数范围内
    if (bias >= -2048 && bias < 2048) {
        riscv_ofs << "\tsw " << rs << ", " << bias << "(" << base << ")" << endl;
    }
    else {
        auto reg = register_manager.tmp_reg();
        _li(reg, bias);
        _add(reg, base, reg);
        riscv_ofs << "\tsw " << rs << ", " << "(" << reg << ")" << endl;
    }
}

/**
 * @brief 生成 bnez 指令，即 if (cond != 0) goto label
 * @param[in] cond 条件寄存器
 * @param[in] label 跳转目标标签
 * @note 会生成多个标签，将短跳转转为长跳转，避免跳转范围限制
 */
void Riscv::_bnez(const string& cond, const string& label) {
    auto target_1 = context_manager.get_branch_label();
    auto target_2 = context_manager.get_branch_end_label();
    riscv_ofs << "\tbnez " << cond << ", " << target_1 << endl;
    _jump(target_2);
    _label(target_1);
    _jump(label);
    _label(target_2);
}

/**
 * @brief 生成 beqz 指令，即 if (cond == 0) goto label
 * @param[in] cond 条件寄存器
 * @param[in] label 跳转目标标签
 * @note 会生成多个标签，将短跳转转为长跳转，避免跳转范围限制
 */
void Riscv::_beqz(const string& cond, const string& label) {
    auto target_1 = context_manager.get_branch_label();
    auto target_2 = context_manager.get_branch_end_label();
    riscv_ofs << "\tbeqz " << cond << ", " << target_1 << endl;
    _jump(target_2);
    _label(target_1);
    _jump(label);
    _label(target_2);
}

/**
 * @brief 生成 j（跳转）指令，即 goto label
 * @param[in] label 跳转目标标签
 */
void Riscv::_jump(const string& label) {
    riscv_ofs << "\tj " << label << endl;
}

/**
 * @brief 将一个值（value）存储到栈中 `stack_map` 中，这个值在后续编译时会使用
 * @param[in] value 指令结果
 * @param[in] bias 偏移量
 * @note 与 _sw 不同，_sw 是生成存储指令，而 push 是真的在表中记录
 * @note 请参见 visit(koopa_raw_value_t) 何时会传递 value
 */
void Context::push(const koopa_raw_value_t& value, int bias) {
    // string msg = "bias: " + to_string(bias) + " stack_size: " + to_string(stack_size);
    // cout << msg << endl;
    assert(bias < stack_size);
    stack_map[value] = bias;
    stack_used += 4;
}

/**
 * @brief 创建一个 Context 对象
 * @param[in] name 函数名
 * @param[in] stack_size 栈空间大小
 */
void ContextManager::create_context(const string& name, int stack_size) {
    context_map[name] = Context(stack_size);
}

/**
 * @brief 获取一个 Context 对象
 * @param[in] name 函数名
 * @return 对应的 Context 对象
 */
Context& ContextManager::get_context(const string& name) {
    return context_map[name];
}

/**
 * @brief 创建一个全局变量，并增加全局变量计数器
 * @param[in] value 全局变量
 */
void ContextManager::create_global(const koopa_raw_value_t& value) {
    auto global_name = "global_" + to_string(global_count);
    global_count++;
    global_map[value] = global_name;
}

/**
 * @brief 获取一个全局变量的名字
 * @param[in] value 全局变量
 * @return 全局变量的名字
 */
string ContextManager::get_global(const koopa_raw_value_t& value) {
    return global_map[value];
}

/**
 * @brief 获取一个分支标签，并增加分支计数器
 * @return 分支标签
 */
string ContextManager::get_branch_label() {
    return "branch_" + to_string(branch_count++);
}

/**
 * @brief 获取一个分支结束标签
 * @return 分支结束标签
 */
string ContextManager::get_branch_end_label() {
    return "branch_end_" + to_string(branch_count);
}

/**
 * @brief 获取当前寄存器
 * @return 当前寄存器
 */
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

/**
 * @brief 获取一个新的寄存器，并增加寄存器计数器
 * @return 新的寄存器
 */
string RegisterManager::new_reg() {
    string reg = cur_reg();
    reg_count++;
    return reg;
}

/**
 * @brief 获取一个临时寄存器，调用完后不会增加寄存器计数器
 * @return 临时寄存器
 */
string RegisterManager::tmp_reg() {
    reg_count++;
    auto ret = cur_reg();
    reg_count--;
    return ret;
}

/**
 * @brief 获取一个值对应的寄存器
 * @param[in] value 值
 * @return 是否额外使用寄存器，若可以推断出值是 0，则不使用寄存器，否则使用寄存器
 */
bool RegisterManager::get_operand_reg(const koopa_raw_value_t& value) {
    // ---[DEBUG]---
    // 打印值的类型
    // printf("get_operand_reg: %s\n", koopaRawValueTagToString(value->kind.tag).c_str());
    // ---[DEBUG END]---
    // 运算数为整数
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
    // 运算数为 call 指令的返回值
    else if (value->kind.tag == KOOPA_RVT_CALL) {
        reg_map[value] = new_reg();
        riscv._lw(reg_map[value], "sp", context.stack_map[value]);
        return true;
    }
    // 运算数为函数参数
    else if (value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
        auto index = value->kind.data.func_arg_ref.index;
        // 前 8 个参数放在 a0 到 a7 寄存器中
        if (index < 8) {
            reg_map[value] = "a" + to_string(index);
        }
        // 再后面的参数要从栈上找
        else {
            // 先获取当前栈帧大小
            reg_map[value] = new_reg();
            int stack_size = context.stack_size;
            int offset = 4 * (index - 8);
            // 从上一个栈帧中获取
            riscv._lw(reg_map[value], "sp", stack_size + offset);
        }
        return true;
    }
    // 其他情况，报错
    else {
        auto msg = "Invalid operand: " + koopaRawValueTagToString(value->kind.tag);
        assert(false && msg.c_str());
    }
    return true;
}

/**
 * @brief 重置寄存器计数器
 */
void RegisterManager::reset() {
    reg_count = 0;
}