#include "include/ast.hpp"

// 全局符号表
SymbolTable global_symbol_table;
// 局部符号表
SymbolTable* local_symbol_table = &global_symbol_table;
// 全局环境管理器
EnvironmentManager environment_manager;

/**
 * @brief 打印根节点 ProgramAST
 * @return
 */
Result ProgramAST::print() const {
    // 初始化库函数
    init_lib();
    // 遍历所有单元
    for (auto& comp_unit : comp_units) {
        comp_unit->print();
    }
    return Result();
}

/**
 * @brief 打印函数定义节点 FuncDefAST
 * @return
 */
Result FuncDefAST::print() const {
    koopa_ofs << endl;
    // 函数体内必然非全局环境
    environment_manager.is_global = false;
    // 保存当前局部符号表
    SymbolTable* parent_symbol_table = local_symbol_table;
    // 创建新的局部符号表
    local_symbol_table = new SymbolTable();
    // 设置父符号表
    local_symbol_table->set_parent(parent_symbol_table);
    // 清空全局环境管理器的 is_symbol_allocated，因为不同函数体内是独立的
    environment_manager.is_symbol_allocated.clear();
    // 清空临时寄存器计数器
    environment_manager.temp_count = 0;
    // 输出函数声明
    koopa_ofs << "fun @" << ident;
    koopa_ofs << "(";
    // 输出函数参数
    if (func_f_params) {
        bool is_first = true;
        // 遍历所有函数参数
        for (auto& item : *func_f_params) {
            if (is_first) {
                is_first = false;
            }
            else {
                koopa_ofs << ", ";
            }
            ((FuncFParamAST*)item.get())->as_param();
        }
    }
    koopa_ofs << ")";
    // 输出函数返回类型
    if (func_type == FuncType::INT) {
        koopa_ofs << ": i32";
        // 记录这个函数有返回值
        environment_manager.is_func_return[ident] = true;
    }
    else {
        // 记录这个函数没有返回值
        environment_manager.is_func_return[ident] = false;
    }
    // 输出函数体
    koopa_ofs << " {" << endl;
    koopa_ofs << "%" << ident << "_entry:" << endl;
    // 首先打印参数
    for (auto& item : *func_f_params) {
        item->print();
    }
    // 然后打印函数体内语句
    block->print();
    // 最后打印返回语句，保证函数最后一句话是 ret
    if (func_type == FuncType::INT) {
        koopa_ofs << "\tret 0" << endl;
    }
    else {
        koopa_ofs << "\tret" << endl;
    }
    koopa_ofs << "}" << endl;
    // 恢复父符号表
    delete local_symbol_table;
    local_symbol_table = parent_symbol_table;
    // 恢复全局环境状态
    environment_manager.is_global = true;
    return Result();
}

/**
 * @brief 在函数签名中打印函数参数，输出行内字符串
 * @note 形如 @ident: i32 / @arr: *i32 / @arr: *[i32, 10]
 */
void FuncFParamAST::as_param() const {
    // 如果是数组参数
    if (is_array) {
        koopa_ofs << "@" << ident << ": *";
        // 准备数组索引
        vector<int> indices;
        for (auto& item : *array_index) {
            indices.push_back(item->print().value);
        }
        // 格式化数组类型
        format_array_type(indices);
    }
    // 如果是普通参数
    else {
        koopa_ofs << "@" << ident << ": i32";
    }
}

/**
 * @brief 在函数体内打印函数参数
 * @return
 * @note 输出形如
 * @note %x = alloc i32
 * @note store @x, %x
 */
Result FuncFParamAST::print() const {
    // 在当前层级符号表中分配变量
    string ident_with_suffix = local_symbol_table->assign(ident);
    // 如果是数组参数
    if (is_array) {
        // 形如 @arr = alloc *[[i32, 3], 2]
        koopa_ofs << "\t@" << ident_with_suffix << " = alloc *";
        // 准备数组索引
        vector<int> indices;
        for (auto& item : *array_index) {
            indices.push_back(item->print().value);
        }
        // 格式化数组类型
        format_array_type(indices);
        koopa_ofs << endl;
        // 在当前层级符号表中创建数组类型变量
        // 由于传参必定是指针，默认有一个 []，所以维度数需要 +1
        local_symbol_table->create(ident_with_suffix, PTR_(array_index->size() + 1));
    }
    // 如果是普通参数
    else {
        // 形如 @x = alloc i32
        koopa_ofs << "\t@" << ident_with_suffix << " = alloc i32" << endl;
        // 在当前层级符号表中创建变量
        local_symbol_table->create(ident_with_suffix, VAR_);
    }
    // 形如 store @x, %x
    koopa_ofs << "\tstore @" << ident << ", @" << ident_with_suffix << endl;
    return Result();
}

Result BlockAST::print() const {
    SymbolTable* parent_symbol_table = local_symbol_table;
    local_symbol_table = new SymbolTable();
    local_symbol_table->set_parent(parent_symbol_table);
    for (auto& item : block_items) {
        if (!local_symbol_table->is_returned) {
            item->print();
        }
    }
    delete local_symbol_table;
    local_symbol_table = parent_symbol_table;
    return Result();
}

Result ConstDeclAST::print() const {
    for (auto& item : const_defs) {
        item->print();
    }
    return Result();
}

/**
 * @brief TODO
 */
Result ConstDefAST::print() const {
    // 在当前层级符号表中分配常量名
    string ident_with_suffix = local_symbol_table->assign(ident);
    // 数组常量
    if (array_index->size() > 0) {
        // 准备数组索引
        vector<int> index_results;
        for (auto& item : *array_index) {
            Result index_result = item->print();
            index_results.push_back(index_result.value);
        }
        // 输出类型声明
        print_array_type(ident_with_suffix, index_results);
        // 判断是否初始化
        // 给定了初始化列表
        if (value) {
            ((ConstInitValAST*)value.get())->print(ident_with_suffix, index_results);
        }
        // 没有给定初始化列表
        else {
            // 全局数组
            if (environment_manager.is_global) {
                koopa_ofs << ", zeroinit" << endl;
            }
            // 局部数组
            else {
                koopa_ofs << endl;
            }
        }
        // 在当前层级符号表中创建数组类型常量
        local_symbol_table->create(ident_with_suffix, ARR_(array_index->size()));
        return Result();
    }
    // 全局 / 局部非数组常量
    else {
        // 计算常量值
        Result value_result = value->print();
        // 在当前层级符号表中创建常量
        local_symbol_table->create(ident_with_suffix, VAL_(value_result.value));
        return Result();
    }
}

void ConstInitValAST::init(const vector<int>& indices, int*& arr, int& cur, int align) {
    int indices_size = indices.size();
    int product = 1;
    deque<int> steps;
    for (int i = indices_size - 1;i >= 0;i--) {
        product *= indices[i];
        steps.push_front(product);
    }
    // // 打印 steps
    // koopa_ofs << "\nsteps: ";
    // for (auto& item : steps) {
    //   koopa_ofs << item << " ";
    // }
    // koopa_ofs << endl;

    // 判断 init_values 是否为空
    if (init_values->empty()) {
        arr[cur++] = 0;
    }
    else {
        for (auto& item : *init_values) {
            auto it = (ConstInitValAST*)(item.get());
            // 如果是整数
            if (it->const_exp) {
                Result res = it->print();
                arr[cur++] = res.value;
            }
            // 如果是初始化列表
            else if (it->init_values) {
                // 从前到后遍历 step，看是否能整除
                // bool is_first = true;
                for (auto& step : steps) {
                    if (step >= align) {
                        continue;
                    }
                    // 找到正确的 step
                    if (cur % step == 0) {
                        it->init(indices, arr, cur, step);
                        break;
                    }
                }
            }
        }
    }
    // 对齐到 align
    // koopa_ofs << "\nalign: " << align << endl;
    while (cur % align != 0) {
        arr[cur++] = 0;
    }
}

Result ConstInitValAST::print(const string& ident, const vector<int>& indices) {
    int total = 1;
    for (auto& item : indices) {
        total *= item;
    }
    int* array = new int[total];
    int cur = 0;
    init(indices, array, cur, total);
    int index = 0;
    vector<int> bases;
    bases.push_back(-1);
    print_array(ident, indices, array, 0, index, bases);
    delete[] array;
    return Result();
}

Result ConstInitValAST::print() const {
    if (const_exp) {
        return (*const_exp)->print();
    }
    else {
        for (auto& item : *init_values) {
            item->print();
        }
    }
    return Result();
}

Result ConstExpAST::print() const {
    return exp->print();
}

Result VarDeclAST::print() const {
    for (auto& item : var_defs) {
        item->print();
    }
    return Result();
}

Result VarDefAST::print() const {
    // 在当前层级符号表中分配变量
    string ident_with_suffix = local_symbol_table->assign(ident);
    // 数组变量
    if (array_index->size() > 0) {
        // 准备数组索引
        vector<int> index_results;
        for (auto& item : *array_index) {
            Result index_result = item->print();
            index_results.push_back(index_result.value);
        }
        // 输出类型声明
        print_array_type(ident_with_suffix, index_results);
        // 判断是否初始化
        if (value) {
            ((InitValAST*)(*value).get())->print(ident_with_suffix, index_results);
        }
        else {
            if (environment_manager.is_global) {
                koopa_ofs << ", zeroinit" << endl;
            }
            else {
                koopa_ofs << endl;
            }
        }
        // 在当前层级符号表中创建变量
        local_symbol_table->create(ident_with_suffix, ARR_(array_index->size()));
        return Result();
    }
    // 非数组变量
    else {
        // 全局变量
        if (environment_manager.is_global) {
            // 判断是否初始化
            if (value) {
                Result value_result = (*value)->print();
                assert(value_result.type == Result::Type::IMM);
                koopa_ofs << "global @" << ident_with_suffix << " = alloc i32, " << value_result << endl;
            }
            else {
                koopa_ofs << "global @" << ident_with_suffix << " = alloc i32, zeroinit" << endl;
            }
        }
        // 局部变量
        else {
            // 判断是否需要生成 alloc 指令
            if (!environment_manager.is_symbol_allocated[ident_with_suffix]) {
                koopa_ofs << "\t@" << ident_with_suffix << " = alloc i32" << endl;
                environment_manager.is_symbol_allocated[ident_with_suffix] = true;
            }
            // 若初始值不为空，则生成 store 指令
            if (value) {
                Result value_result = (*value)->print();
                koopa_ofs << "\tstore " << value_result << ", @" << ident_with_suffix << endl;
            }
        }
        // 在当前层级符号表中创建变量
        local_symbol_table->create(ident_with_suffix, VAR_);
        return Result();
    }
}

void InitValAST::init(const vector<int>& indices, int* arr, int& cur, int align) {
    int indices_size = indices.size();
    int product = 1;
    deque<int> steps;
    for (int i = indices_size - 1;i >= 0;i--) {
        product *= indices[i];
        steps.push_front(product);
    }
    // 判断 init_values 是否为空
    if (init_values->empty()) {
        arr[cur++] = 0;
    }
    else {
        for (auto& item : *init_values) {
            auto it = (InitValAST*)(item.get());
            // 如果是整数
            if (it->exp) {
                Result res = it->print();
                arr[cur++] = res.value;
            }
            // 如果是初始化列表
            else if (it->init_values) {
                // 从前到后遍历 step，看是否能整除
                // bool is_first = true;
                for (auto& step : steps) {
                    if (step >= align) {
                        continue;
                    }
                    // koopa_ofs << "here: " << step << " " << cur << endl;
                    // 找到正确的 step
                    if (cur % step == 0) {
                        it->init(indices, arr, cur, step);
                        break;
                    }
                }
            }
        }
    }
    // 对齐到 align
    // koopa_ofs << "\nalign: " << align << endl;
    while (cur % align != 0) {
        arr[cur++] = 0;
    }
}

Result InitValAST::print(const string& ident, const vector<int>& indices) {
    int total = 1;
    for (auto& item : indices) {
        total *= item;
    }
    int* array = new int[total];
    int cur = 0;
    init(indices, array, cur, total);
    int index = 0;
    vector<int> bases;
    bases.push_back(-1);
    print_array(ident, indices, array, 0, index, bases);
    delete[] array;
    return Result();
}

Result InitValAST::print() const {
    if (exp) {
        return (*exp)->print();
    }
    return Result();
}

Result StmtIfAST::print() const {
    // 先计算表达式结果
    Result exp_result = exp->print();
    // 创建新的符号表，避免 if 语句中的单句 return 修改当前块 is_returned 
    SymbolTable* parent_symbol_table = local_symbol_table;
    local_symbol_table = new SymbolTable();
    local_symbol_table->set_parent(parent_symbol_table);
    // 准备标签
    string then_label = environment_manager.get_then_label();
    string else_label = environment_manager.get_else_label();
    string end_label = environment_manager.get_end_label();
    environment_manager.add_if_else_count();
    // 根据是否存在 else 语句进行分支处理
    if (else_stmt) {
        koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << else_label << endl;
        koopa_ofs << then_label << ":" << endl;
        then_stmt->print();
        koopa_ofs << "\tjump " << end_label << endl;
        koopa_ofs << else_label << ":" << endl;
        (*else_stmt)->print();
        koopa_ofs << "\tjump " << end_label << endl;
    }
    else {
        koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << end_label << endl;
        koopa_ofs << then_label << ":" << endl;
        then_stmt->print();
        koopa_ofs << "\tjump " << end_label << endl;
    }
    koopa_ofs << end_label << ":" << endl;
    // 恢复符号表
    delete local_symbol_table;
    local_symbol_table = parent_symbol_table;

    return Result();
}

Result StmtWhileAST::print() const {
    // 准备标签
    string entry_label = environment_manager.get_while_entry_label();
    string body_label = environment_manager.get_while_body_label();
    string end_label = environment_manager.get_while_end_label();
    auto old_while_current = environment_manager.get_while_current();
    environment_manager.set_while_current(environment_manager.get_while_count());
    environment_manager.add_while_count();

    // 生成 while 循环
    koopa_ofs << "\tjump " << entry_label << endl;
    koopa_ofs << entry_label << ":" << endl;
    Result exp_result = exp->print();
    // 创建新的符号表，避免 while 语句中的单句 return 修改当前块 is_returned
    SymbolTable* parent_symbol_table = local_symbol_table;
    local_symbol_table = new SymbolTable();
    local_symbol_table->set_parent(parent_symbol_table);
    koopa_ofs << "\tbr " << exp_result << ", " << body_label << ", " << end_label << endl;
    koopa_ofs << body_label << ":" << endl;
    stmt->print();
    koopa_ofs << "\tjump " << entry_label << endl;
    koopa_ofs << end_label << ":" << endl;
    // 恢复符号表
    delete local_symbol_table;
    local_symbol_table = parent_symbol_table;
    environment_manager.set_while_current(old_while_current);
    return Result();
}

Result StmtBreakAST::print() const {
    koopa_ofs << "\tjump " << environment_manager.get_while_end_label(true) << endl;
    auto jump_label = environment_manager.get_jump_label();
    koopa_ofs << jump_label << ":" << endl;
    return Result();
}

Result StmtContinueAST::print() const {
    koopa_ofs << "\tjump " << environment_manager.get_while_entry_label(true) << endl;
    auto jump_label = environment_manager.get_jump_label();
    koopa_ofs << jump_label << ":" << endl;
    return Result();
}

Result StmtAssignAST::print() const {
    auto l_val_ast = (LValAST*)l_val.get();
    // 获取变量名
    auto ident = l_val_ast->ident;
    // 获取变量名在符号表中的位置，可能需要向上级符号表查找
    string ident_with_suffix = local_symbol_table->locate(ident);
    assert(local_symbol_table->exist(ident_with_suffix));
    // 获取变量
    auto symbol = local_symbol_table->read(ident_with_suffix);
    // 计算表达式结果并存储到变量中
    Result exp_result = exp->print();
    // 判断是否为数组类型
    /*
    %0 = getelemptr @arr3_2, 0
    %1 = getelemptr %0, 2
    %2 = getelemptr %1, 3 */
    if (l_val_ast->array_index->size() > 0) {
        if (symbol.type == Symbol::Type::ARR) {
            for (int i = 0;i < l_val_ast->array_index->size();i++) {
                auto prev_reg = CUR_REG_;
                auto index = (*l_val_ast->array_index)[i]->print();
                if (i == 0) {
                    koopa_ofs << "\t" << NEW_REG_ << " = getelemptr @" << ident_with_suffix << ", " << index << endl;
                }
                else {
                    koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", " << index << endl;
                }
            }
            koopa_ofs << "\tstore " << exp_result << ", " << CUR_REG_ << endl;
        }
        else if (symbol.type == Symbol::Type::PTR) {
            koopa_ofs << "\t" << NEW_REG_ << " = load @" << ident_with_suffix << endl;
            for (int i = 0;i < l_val_ast->array_index->size();i++) {
                auto prev_reg = CUR_REG_;
                auto index = (*l_val_ast->array_index)[i]->print();
                if (i == 0) {
                    koopa_ofs << "\t" << NEW_REG_ << " = getptr " << prev_reg << ", " << index << endl;
                }
                else {
                    koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", " << index << endl;
                }
            }
            koopa_ofs << "\tstore " << exp_result << ", " << CUR_REG_ << endl;
        }
    }
    else {
        koopa_ofs << "\tstore " << exp_result << ", @" << ident_with_suffix << endl;
    }
    return Result();
}

Result StmtExpAST::print() const {
    // 若表达式不为空，则计算表达式结果
    if (exp) {
        return (*exp)->print();
    }
    return Result();
}

Result StmtReturnAST::print() const {
    // 若表达式不为空，则计算表达式结果
    if (exp) {
        Result exp_result = (*exp)->print();
        koopa_ofs << "\tret " << exp_result << endl;
    }
    // 若表达式为空，则单句返回即可
    else {
        koopa_ofs << "\tret" << endl;
    }
    // 设置当前块 is_returned 为 true
    local_symbol_table->is_returned = true;
    // 设置返回结束标签，这样可以避免一个标号末尾出现多句 ret / br / jump 的情况
    auto jump_label = environment_manager.get_jump_label();
    koopa_ofs << jump_label << ":" << endl;
    return Result();
}

Result LValAST::print() const {
    // 获取变量名在符号表中的位置，然后读取其值
    string ident_with_suffix = local_symbol_table->locate(ident);
    assert(local_symbol_table->exist(ident_with_suffix));
    auto symbol = local_symbol_table->read(ident_with_suffix);
    // 若变量是变量，则使用 load 指令读取其值
    if (symbol.type == Symbol::Type::VAR) {
        Result result = NEW_REG_;
        koopa_ofs << "\t" << result << " = load @" << ident_with_suffix << endl;
        return result;
    }
    // 若变量是常量，则直接返回常量值
    else if (symbol.type == Symbol::Type::VAL) {
        return IMM_(symbol.value);
    }
    else if (symbol.type == Symbol::Type::ARR) {
        // 准备数组索引
        vector<Result> indices;
        for (auto& item : *array_index) {
            indices.push_back(item->print());
        }
        // 打印数组索引
        // koopa_ofs << "indices: ";
        // for (auto& item : indices) {
        //   koopa_ofs << item << " ";
        // }
        // koopa_ofs << endl;
        // 获取数组维度
        int set_dims = 0;
        for (int i = 0;i < indices.size();i++) {
            set_dims += 1;
            auto prev_reg = CUR_REG_;
            if (i == 0) {
                koopa_ofs << "\t" << NEW_REG_ << " = getelemptr @" << ident_with_suffix << ", " << indices[i] << endl;
            }
            else {
                koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", " << indices[i] << endl;
            }
        }
        auto prev_reg = CUR_REG_;
        // koopa_ofs << "set_dims: " << set_dims << ", symbol.value: " << symbol.value << endl;
        if (set_dims == symbol.value) {
            koopa_ofs << "\t" << NEW_REG_ << " = load " << prev_reg << endl;
        }
        // 处理完全没指定的情况
        else if (set_dims == 0) {
            koopa_ofs << "\t" << NEW_REG_ << " = getelemptr @" << ident_with_suffix << ", 0" << endl;
        }
        else {
            koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", 0" << endl;
        }
        return CUR_REG_;
    }
    else if (symbol.type == Symbol::Type::PTR) {
        // 准备数组索引
        vector<Result> indices;
        for (auto& item : *array_index) {
            indices.push_back(item->print());
        }
        // 获取数组维度
        int set_dims = 0;
        koopa_ofs << "\t" << NEW_REG_ << " = load @" << ident_with_suffix << endl;
        for (int i = 0;i < indices.size();i++) {
            set_dims += 1;
            auto prev_reg = CUR_REG_;
            if (i == 0) {
                koopa_ofs << "\t" << NEW_REG_ << " = getptr " << prev_reg << ", " << indices[i] << endl;
            }
            else {
                koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", " << indices[i] << endl;
            }
        }
        auto prev_reg = CUR_REG_;
        // koopa_ofs << "set_dims: " << set_dims << ", symbol.value: " << symbol.value << endl;
        if (set_dims == symbol.value) {
            koopa_ofs << "\t" << NEW_REG_ << " = load " << prev_reg << endl;
        }
        else if (set_dims != 0) {
            koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", 0" << endl;
        }
        // 对于完全没指定的情况，前面最开始使用的 load 指令就是结果了
        return CUR_REG_;
    }
    else {
        assert(false);
    }
}

Result ExpAST::print() const {
    return l_or_exp->print();
}

Result LOrExpAST::print() const {
    return l_and_exp->print();
}

Result LAndExpAST::print() const {
    return eq_exp->print();
}

Result LExpWithOpAST::print() const {
    // 先计算左表达式结果
    Result lhs = left->print();

    if (logical_op == LogicalOp::LOGICAL_OR) {
        // 左侧为立即数
        if (lhs.type == Result::Type::IMM) {
            if (lhs.value != 0) {
                return IMM_(1);
            }
            else {
                Result rhs = right->print();
                if (rhs.type == Result::Type::IMM) {
                    return IMM_(rhs.value != 0);
                }
                else {
                    return rhs;
                }
            }
        }
        // 左侧不为立即数
        auto true_label = environment_manager.get_short_true_label();
        auto false_label = environment_manager.get_short_false_label();
        auto end_label = environment_manager.get_short_end_label();
        auto result = environment_manager.get_short_result_reg();
        environment_manager.add_short_circuit_count();

        // 判断是否需要生成 alloc 指令
        if (!environment_manager.is_symbol_allocated[result]) {
            koopa_ofs << "\t" << result << " = alloc i32" << endl;
            environment_manager.is_symbol_allocated[result] = true;
        }

        koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;
        koopa_ofs << true_label << ":" << endl;
        koopa_ofs << "\t" << "store 1, " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        koopa_ofs << false_label << ":" << endl;
        Result rhs = right->print();
        Result temp_1 = NEW_REG_;
        Result temp_2 = NEW_REG_;
        koopa_ofs << "\t" << temp_1 << " = or " << rhs << ", 0" << endl;
        koopa_ofs << "\t" << temp_2 << " = ne " << temp_1 << ", 0" << endl;
        koopa_ofs << "\t" << "store " << temp_2 << ", " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;
        koopa_ofs << end_label << ":" << endl;
        Result result_reg = NEW_REG_;
        koopa_ofs << "\t" << result_reg << " = load " << result << endl;

        return result_reg;
    }
    else if (logical_op == LogicalOp::LOGICAL_AND) {
        // 左侧为立即数
        if (lhs.type == Result::Type::IMM) {
            if (lhs.value == 0) {
                return IMM_(0);
            }
            else {
                Result rhs = right->print();
                if (rhs.type == Result::Type::IMM) {
                    return IMM_(rhs.value != 0);
                }
                else {
                    return rhs;
                }
            }
        }
        // 左侧不为立即数
        auto true_label = environment_manager.get_short_true_label();
        auto false_label = environment_manager.get_short_false_label();
        auto end_label = environment_manager.get_short_end_label();
        auto result = environment_manager.get_short_result_reg();
        environment_manager.add_short_circuit_count();

        // 判断是否需要生成 alloc 指令
        if (!environment_manager.is_symbol_allocated[result]) {
            koopa_ofs << "\t" << result << " = alloc i32" << endl;
            environment_manager.is_symbol_allocated[result] = true;
        }

        koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;
        koopa_ofs << false_label << ":" << endl;
        koopa_ofs << "\t" << "store 0, " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        koopa_ofs << true_label << ":" << endl;
        Result rhs = right->print();
        Result temp_1 = NEW_REG_;
        Result temp_2 = NEW_REG_;
        Result temp_3 = NEW_REG_;
        koopa_ofs << "\t" << temp_1 << " = ne " << lhs << ", 0" << endl;
        koopa_ofs << "\t" << temp_2 << " = ne " << rhs << ", 0" << endl;
        koopa_ofs << "\t" << temp_3 << " = and " << temp_1 << ", " << temp_2 << endl;
        koopa_ofs << "\t" << "store " << temp_3 << ", " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;
        koopa_ofs << end_label << ":" << endl;
        Result result_reg = NEW_REG_;
        koopa_ofs << "\t" << result_reg << " = load " << result << endl;

        return result_reg;
    }
    else {
        assert(false);
    }
}

EqExpWithOpAST::EqOp EqExpWithOpAST::convert(const string& op) const {
    if (op == "==") {
        return EqOp::EQ;
    }
    else if (op == "!=") {
        return EqOp::NEQ;
    }
    throw runtime_error("Invalid operator: " + op);
}

Result EqExpAST::print() const {
    return rel_exp->print();
}

Result EqExpWithOpAST::print() const {
    // 先计算左右表达式结果
    Result lhs = left->print();
    Result rhs = right->print();
    // 若左右表达式结果均为常量，则直接返回常量结果
    if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
        switch (eq_op) {
        case EqOp::EQ:
            return IMM_(lhs.value == rhs.value);
        case EqOp::NEQ:
            return IMM_(lhs.value != rhs.value);
        default:
            assert(false);
        }
    }
    // 若左右表达式结果不均为常量，则使用临时变量计算结果
    else {
        Result result = NEW_REG_;
        switch (eq_op) {
        case EqOp::EQ:
            koopa_ofs << "\t" << result << " = eq " << lhs << ", " << rhs << endl;
            break;
        case EqOp::NEQ:
            koopa_ofs << "\t" << result << " = ne " << lhs << ", " << rhs << endl;
            break;
        default:
            assert(false);
        }
        return result;
    }
}

Result RelExpAST::print() const {
    return add_exp->print();
}

RelExpWithOpAST::RelOp RelExpWithOpAST::convert(const string& op) const {
    if (op == "<=") {
        return RelOp::LE;
    }
    else if (op == ">=") {
        return RelOp::GE;
    }
    else if (op == "<") {
        return RelOp::LT;
    }
    else if (op == ">") {
        return RelOp::GT;
    }
    throw runtime_error("Invalid operator: " + op);
}

Result RelExpWithOpAST::print() const {
    // 先计算左右表达式结果
    Result lhs = left->print();
    Result rhs = right->print();
    // 若左右表达式结果均为常量，则直接返回常量结果
    if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
        switch (rel_op) {
        case RelOp::LE:
            return IMM_(lhs.value <= rhs.value);
        case RelOp::GE:
            return IMM_(lhs.value >= rhs.value);
        case RelOp::LT:
            return IMM_(lhs.value < rhs.value);
        case RelOp::GT:
            return IMM_(lhs.value > rhs.value);
        default:
            assert(false);
        }
    }
    // 若左右表达式结果不均为常量，则使用临时变量计算结果
    else {
        Result result = NEW_REG_;
        switch (rel_op) {
        case RelOp::LE:
            koopa_ofs << "\t" << result << " = le " << lhs << ", " << rhs << endl;
            break;
        case RelOp::GE:
            koopa_ofs << "\t" << result << " = ge " << lhs << ", " << rhs << endl;
            break;
        case RelOp::LT:
            koopa_ofs << "\t" << result << " = lt " << lhs << ", " << rhs << endl;
            break;
        case RelOp::GT:
            koopa_ofs << "\t" << result << " = gt " << lhs << ", " << rhs << endl;
            break;
        default:
            assert(false);
        }
        return result;
    }
}

Result AddExpAST::print() const {
    return mul_exp->print();
}

AddExpWithOpAST::AddOp AddExpWithOpAST::convert(const string& op) const {
    if (op == "+") {
        return AddOp::ADD;
    }
    else if (op == "-") {
        return AddOp::SUB;
    }
    throw runtime_error("Invalid operator: " + op);
}

Result AddExpWithOpAST::print() const {
    // 先计算左右表达式结果
    Result lhs = left->print();
    Result rhs = right->print();
    // 若左右表达式结果均为常量，则直接返回常量结果
    if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
        switch (add_op) {
        case AddOp::ADD:
            return IMM_(lhs.value + rhs.value);
        case AddOp::SUB:
            return IMM_(lhs.value - rhs.value);
        default:
            assert(false);
        }
    }
    // 若左右表达式结果不均为常量，则使用临时变量计算结果
    else {
        Result result = NEW_REG_;
        switch (add_op) {
        case AddOp::ADD:
            koopa_ofs << "\t" << result << " = add " << lhs << ", " << rhs << endl;
            break;
        case AddOp::SUB:
            koopa_ofs << "\t" << result << " = sub " << lhs << ", " << rhs << endl;
            break;
        default:
            assert(false);
        }
        return result;
    }
}

Result MulExpAST::print() const {
    return unary_exp->print();
}

MulExpWithOpAST::MulOp MulExpWithOpAST::convert(const string& op) const {
    if (op == "*") {
        return MulOp::MUL;
    }
    else if (op == "/") {
        return MulOp::DIV;
    }
    else if (op == "%") {
        return MulOp::MOD;
    }
    throw runtime_error("Invalid operator: " + op);
}

Result MulExpWithOpAST::print() const {
    // 先计算左右表达式结果
    Result lhs = left->print();
    Result rhs = right->print();
    // 若左右表达式结果均为常量，则直接返回常量结果
    if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
        switch (mul_op) {
        case MulOp::MUL:
            return IMM_(lhs.value * rhs.value);
        case MulOp::DIV:
            return IMM_(lhs.value / rhs.value);
        case MulOp::MOD:
            return IMM_(lhs.value % rhs.value);
        default:
            assert(false);
        }
    }
    // 若左右表达式结果不均为常量，则使用临时变量计算结果
    else {
        Result result = NEW_REG_;
        switch (mul_op) {
        case MulOp::MUL:
            koopa_ofs << "\t" << result << " = mul " << lhs << ", " << rhs << endl;
            break;
        case MulOp::DIV:
            koopa_ofs << "\t" << result << " = div " << lhs << ", " << rhs << endl;
            break;
        case MulOp::MOD:
            koopa_ofs << "\t" << result << " = mod " << lhs << ", " << rhs << endl;
            break;
        default:
            assert(false);
        }
        return result;
    }
}

Result UnaryExpAST::print() const {
    return primary_exp->print();
}

UnaryExpWithOpAST::UnaryOp UnaryExpWithOpAST::convert(const string& op) const {
    if (op == "+") {
        return UnaryOp::POSITIVE;
    }
    else if (op == "-") {
        return UnaryOp::NEGATIVE;
    }
    else if (op == "!") {
        return UnaryOp::NOT;
    }
    throw runtime_error("Invalid operator: " + op);
}

Result UnaryExpWithOpAST::print() const {
    // 先计算表达式结果
    Result unary_exp_result = unary_exp->print();
    // 若表达式结果为常量，则直接返回常量结果
    if (unary_exp_result.type == Result::Type::IMM) {
        switch (unary_op) {
        case UnaryOp::POSITIVE:
            return IMM_(unary_exp_result.value);
        case UnaryOp::NEGATIVE:
            return IMM_(-unary_exp_result.value);
        case UnaryOp::NOT:
            return IMM_(!unary_exp_result.value);
        default:
            assert(false);
        }
    }
    // 若表达式结果为临时变量，则使用临时变量计算结果
    else {
        Result result = NEW_REG_;
        switch (unary_op) {
        case UnaryOp::POSITIVE:
            koopa_ofs << "\t" << result << " = add 0, " << unary_exp_result << endl;
            break;
        case UnaryOp::NEGATIVE:
            koopa_ofs << "\t" << result << " = sub 0, " << unary_exp_result << endl;
            break;
        case UnaryOp::NOT:
            koopa_ofs << "\t" << result << " = eq 0, " << unary_exp_result << endl;
            break;
        default:
            assert(false);
        }
        return result;
    }
}

Result UnaryExpWithFuncCallAST::print() const {
    vector<Result> params;
    for (auto& param : *func_r_params) {
        params.push_back(param->print());
    }

    if (environment_manager.is_func_return[ident]) {
        Result result = NEW_REG_;
        koopa_ofs << "\t" << result << " = call @" << ident << "(";
        for (size_t i = 0; i < params.size(); i++) {
            koopa_ofs << params[i];
            if (i != params.size() - 1) {
                koopa_ofs << ", ";
            }
        }
        koopa_ofs << ")" << endl;
        return result;
    }
    else {
        koopa_ofs << "\tcall @" << ident << "(";
        for (size_t i = 0; i < params.size(); i++) {
            koopa_ofs << params[i];
            if (i != params.size() - 1) {
                koopa_ofs << ", ";
            }
        }
        koopa_ofs << ")" << endl;
        return Result();
    }
}


Result PrimaryExpAST::print() const {
    return exp->print();
}

Result PrimaryExpWithNumberAST::print() const {
    return IMM_(number);
}

Result PrimaryExpWithLValAST::print() const {
    return l_val->print();
}