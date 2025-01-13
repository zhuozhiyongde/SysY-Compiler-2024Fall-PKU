#include "include/ast.hpp"

// 全局符号表
SymbolTable global_symbol_table;
// 局部符号表
SymbolTable* local_symbol_table = &global_symbol_table;
// 全局环境管理器
EnvironmentManager environment_manager;

/**
 * @brief 打印根节点 ProgramAST
 * */
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
 * */
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
 * * @note 输出形如
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
    // 输出形如 store @x, %x 的 store 指令
    koopa_ofs << "\tstore @" << ident << ", @" << ident_with_suffix << endl;
    return Result();
}

/**
 * @brief 打印函数体
 * */
Result BlockAST::print() const {
    // 保存当前局部符号表
    SymbolTable* parent_symbol_table = local_symbol_table;
    // 创建新的局部符号表
    local_symbol_table = new SymbolTable();
    // 设置父符号表
    local_symbol_table->set_parent(parent_symbol_table);
    // 遍历所有语句
    for (auto& item : block_items) {
        // 如果符号表内没有出现返回语句则继续打印
        if (!local_symbol_table->is_returned) {
            item->print();
        }
    }
    // 恢复父符号表
    delete local_symbol_table;
    local_symbol_table = parent_symbol_table;
    return Result();
}

/**
 * @brief 打印常量定义列表
 * */
Result ConstDeclAST::print() const {
    // 遍历常量声明列表
    for (auto& item : const_defs) {
        item->print();
    }
    return Result();
}

/**
 * @brief 打印常量定义
 * */
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
        // 没有给定初始化列表，默认置零
        // 由于是常量，所以必然要初始化掉
        else {
            // 全局数组可以用 zeroinit 初始化
            if (environment_manager.is_global) {
                koopa_ofs << ", zeroinit" << endl;
            }
            // 局部数组不能用 zeroinit 初始化
            else {
                koopa_ofs << endl;
            }
        }
        // 在当前层级符号表中创建数组类型常量，并记录维度数
        local_symbol_table->create(ident_with_suffix, ARR_(array_index->size()));
    }
    // 全局 / 局部非数组常量
    else {
        // 计算常量值
        Result value_result = value->print();
        // 在当前层级符号表中创建常量
        local_symbol_table->create(ident_with_suffix, VAL_(value_result.value));
    }
    return Result();
}

/**
 * @brief 初始化常量数组
 * @param[in] indices 数组维度
 * @param[inout] arr 存放数组值的数组
 * @param[inout] cur 当前索引
 * @param[in] align 对齐数
 */
void ConstInitValAST::init(const vector<int>& indices, int*& arr, int& cur, int align) {
    // 计算对齐粒度向量
    int indices_size = indices.size();
    int product = 1;
    deque<int> steps;
    for (int i = indices_size - 1;i >= 0;i--) {
        product *= indices[i];
        steps.push_front(product);
    }
    // ---[DEBUG]---
    // 打印 steps
    // koopa_ofs << "\nsteps: ";
    // for (auto& item : steps) {
    //   koopa_ofs << item << " ";
    // }
    // koopa_ofs << endl;
    // ---[DEBUG END]---
    // 判断 init_values 是否为空
    // 若为空，则至少要填入一个 0 使得后续对齐可以工作
    // 如果不填的话后续对齐会直接跳过，因为初始的时候必然是对齐了 align 的
    if (init_values->empty()) {
        arr[cur++] = 0;
    }
    else {
        // 从前到后遍历初始化值的列表
        for (auto& item : *init_values) {
            auto it = (ConstInitValAST*)(item.get());
            // 如果是整数，直接放入
            if (it->const_exp) {
                Result res = it->print();
                arr[cur++] = res.value;
            }
            // 如果是初始化列表，递归调用
            else if (it->init_values) {
                // 从前到后遍历 step，看是否能整除
                for (auto& step : steps) {
                    // 保证次级列表的对齐粒度不会大于等于当前的对齐粒度
                    if (step >= align) {
                        continue;
                    }
                    // 通过判断余数，找到正确的 step
                    if (cur % step == 0) {
                        // 递归调用
                        it->init(indices, arr, cur, step);
                        break;
                    }
                }
            }
        }
    }
    // 末尾对齐到 align
    while (cur % align != 0) {
        arr[cur++] = 0;
    }
}

/**
 * @brief 打印常量数组
 * @param[in] ident 常量名
 * @param[in] indices 数组维度
 * */
Result ConstInitValAST::print(const string& ident, const vector<int>& indices) {
    // 计算数组总大小
    int total = 1;
    for (auto& item : indices) {
        total *= item;
    }
    // 分配一个存放初始化值的数组
    int* array = new int[total];
    // 初始化数组
    int cur = 0;
    init(indices, array, cur, total);
    // 打印数组
    // 记录当前打印到的索引
    int index = 0;
    // 记录基址
    vector<int> bases;
    // 基址初始化为 -1，方便后续特判最外层
    bases.push_back(-1);
    print_array(ident, indices, array, 0, index, bases);
    // 释放数组
    delete[] array;
    return Result();
}

/**
 * @brief 打印常量初始化值
 * */
Result ConstInitValAST::print() const {
    // 如果是以最末级的常量初始化，打印之
    if (const_exp) {
        return (*const_exp)->print();
    }
    // 如果是初始化列表，递归调用（其实好像进不到这里）
    else {
        for (auto& item : *init_values) {
            item->print();
        }
    }
    return Result();
}

/**
 * @brief 打印常量表达式
 * */
Result ConstExpAST::print() const {
    return exp->print();
}

/**
 * @brief 打印变量定义列表
 * */
Result VarDeclAST::print() const {
    for (auto& item : var_defs) {
        item->print();
    }
    return Result();
}

/**
 * @brief 打印变量定义
 * */
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
        // 给定了初始化列表
        if (value) {
            ((InitValAST*)(*value).get())->print(ident_with_suffix, index_results);
        }
        // 没有给定初始化列表，默认置零
        else {
            // 全局数组可以用 zeroinit 初始化
            if (environment_manager.is_global) {
                koopa_ofs << ", zeroinit" << endl;
            }
            // 局部数组不能用 zeroinit 初始化
            else {
                koopa_ofs << endl;
            }
        }
        // 在当前层级符号表中创建数组类型变量，并记录维度数
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
            // 没有给定初始化列表，默认置零
            else {
                koopa_ofs << "global @" << ident_with_suffix << " = alloc i32, zeroinit" << endl;
            }
        }
        // 局部变量
        else {
            // 判断是否需要生成 alloc 指令
            // 如果已经分配过，不能再次分配，因为不能有两条同名的 alloc 指令
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

/**
 * @brief 初始化变量数组
 * @param[in] indices 数组维度
 * @param[out] arr 存放数组值的数组
 * @param cur 当前索引
 * @param align 对齐数
 * */
void InitValAST::init(const vector<int>& indices, int* arr, int& cur, int align) {
    // 计算对齐粒度向量
    int indices_size = indices.size();
    int product = 1;
    deque<int> steps;
    for (int i = indices_size - 1;i >= 0;i--) {
        product *= indices[i];
        steps.push_front(product);
    }
    // 判断 init_values 是否为空
    // 若为空，则至少要填入一个 0 使得后续对齐可以工作
    // 如果不填的话后续对齐会直接跳过，因为初始的时候必然是对齐了 align 的
    if (init_values->empty()) {
        arr[cur++] = 0;
    }
    else {
        // 从前到后遍历初始化值的列表
        for (auto& item : *init_values) {
            auto it = (InitValAST*)(item.get());
            // 如果是整数，直接放入
            if (it->exp) {
                Result res = it->print();
                arr[cur++] = res.value;
            }
            // 如果是初始化列表，递归调用
            else if (it->init_values) {
                // 从前到后遍历 step，看是否能整除
                for (auto& step : steps) {
                    // 保证次级列表的对齐粒度不会大于等于当前的对齐粒度
                    if (step >= align) {
                        continue;
                    }
                    // 通过判断余数，找到正确的 step
                    if (cur % step == 0) {
                        it->init(indices, arr, cur, step);
                        break;
                    }
                }
            }
        }
    }
    // 末尾对齐到 align
    while (cur % align != 0) {
        arr[cur++] = 0;
    }
}

/**
 * @brief 打印变量数组
 * @param[in] ident 变量名
 * @param[in] indices 数组维度
 * */
Result InitValAST::print(const string& ident, const vector<int>& indices) {
    // 计算数组总大小
    int total = 1;
    for (auto& item : indices) {
        total *= item;
    }
    // 分配一个存放初始化值的数组
    int* array = new int[total];
    // 初始化数组
    int cur = 0;
    init(indices, array, cur, total);
    // 打印数组
    // 记录当前打印到的索引
    int index = 0;
    // 记录基址
    vector<int> bases;
    // 基址初始化为 -1，方便后续特判最外层
    bases.push_back(-1);
    print_array(ident, indices, array, 0, index, bases);
    // 释放数组
    delete[] array;
    return Result();
}

/**
 * @brief 打印变量初始化值
 * */
Result InitValAST::print() const {
    // 如果是以最末级的真实值初始化，打印之
    if (exp) {
        return (*exp)->print();
    }
    // 如果是初始化列表，递归调用（其实好像进不到这里）
    else {
        for (auto& item : *init_values) {
            item->print();
        }
    }
    return Result();
}

/**
 * @brief 打印 if 语句
 * */
Result StmtIfAST::print() const {
    // 先打印条件表达式，并存储计算得出的条件表达式结果
    Result exp_result = exp->print();
    // 备份是否返回的记录，避免 if 语句中的单句 return 修改当前块 is_returned
    bool backup_is_returned = local_symbol_table->is_returned;
    // 准备标签
    string then_label = environment_manager.get_then_label();
    string else_label = environment_manager.get_else_label();
    string end_label = environment_manager.get_end_label();
    environment_manager.add_if_else_count();
    // 根据是否存在 else 语句进行分支处理
    if (else_stmt) {
        // 生成 br 指令
        koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << else_label << endl;
        // 生成 then 语句块
        koopa_ofs << then_label << ":" << endl;
        then_stmt->print();
        koopa_ofs << "\tjump " << end_label << endl;
        // 生成 else 语句块
        koopa_ofs << else_label << ":" << endl;
        (*else_stmt)->print();
        koopa_ofs << "\tjump " << end_label << endl;
    }
    else {
        // 生成 br 指令
        koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << end_label << endl;
        // 生成 then 语句块
        koopa_ofs << then_label << ":" << endl;
        then_stmt->print();
        koopa_ofs << "\tjump " << end_label << endl;
    }
    // 生成 end 标签
    koopa_ofs << end_label << ":" << endl;
    // 恢复是否返回的记录
    local_symbol_table->is_returned = backup_is_returned;
    return Result();
}

/**
 * @brief 打印 while 语句
 * */
Result StmtWhileAST::print() const {
    // While 循环有两个计数器：
    // 1. 全局 while 循环的计数器 count，用于生成 while 循环的标签
    // 2. 当前 while 循环的计数器 current，用于生成 break 和 continue 的跳转目标标签
    // 使用全局计数器 count 准备标签
    string entry_label = environment_manager.get_while_entry_label();
    string body_label = environment_manager.get_while_body_label();
    string end_label = environment_manager.get_while_end_label();
    // 备份当前 while 循环的计数器 current
    auto old_while_current = environment_manager.get_while_current();
    // 新开了循环，将 current 设置为 count
    environment_manager.set_while_current(environment_manager.get_while_count());
    // 增加 while 循环计数器
    environment_manager.add_while_count();
    // 生成 while 循环
    // 首行生成一条跳转指令，跳转到 while 循环的入口，避免之前最后一条指令是标号，如函数起始处
    koopa_ofs << "\tjump " << entry_label << endl;
    // 生成 while 循环的入口标签
    koopa_ofs << entry_label << ":" << endl;
    // 打印条件表达式，并存储计算得出的条件表达式结果
    Result exp_result = exp->print();
    // 备份是否返回的记录，避免 while 语句中的单句 return 修改当前块 is_returned
    bool backup_is_returned = local_symbol_table->is_returned;
    // 生成条件跳转指令
    koopa_ofs << "\tbr " << exp_result << ", " << body_label << ", " << end_label << endl;
    // 生成 while 循环体
    koopa_ofs << body_label << ":" << endl;
    stmt->print();
    koopa_ofs << "\tjump " << entry_label << endl;
    // 生成 end 标签
    koopa_ofs << end_label << ":" << endl;
    // 恢复是否返回的记录
    local_symbol_table->is_returned = backup_is_returned;
    // 恢复当前 while 循环计数器 current
    environment_manager.set_while_current(old_while_current);
    return Result();
}

/**
 * @brief 打印 break 语句
 * */
Result StmtBreakAST::print() const {
    // 生成跳转指令，跳转到当前 while 循环（即 current 计数器）的结束标签
    koopa_ofs << "\tjump " << environment_manager.get_while_end_label(true) << endl;
    // 生成跳转标签
    auto jump_label = environment_manager.get_jump_label();
    koopa_ofs << jump_label << ":" << endl;
    return Result();
}

/**
 * @brief 打印 continue 语句
 * */
Result StmtContinueAST::print() const {
    // 生成跳转指令，跳转到当前 while 循环（即 current 计数器）的入口标签
    koopa_ofs << "\tjump " << environment_manager.get_while_entry_label(true) << endl;
    // 生成跳转标签
    auto jump_label = environment_manager.get_jump_label();
    koopa_ofs << jump_label << ":" << endl;
    return Result();
}

/**
 * @brief 打印赋值语句
 * @note 注意是赋值语句，不是声明语句，所以不需要在符号表中创建新的记录
 * */
Result StmtAssignAST::print() const {
    // 获取左值
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
    // 判断左值是否为数组相关类型
    // 数组类型
    if (symbol.type == Symbol::Type::ARR) {
        /*
        arr[2][3] = 1 应当翻译为（两次寻址）：
        %0 = getelemptr @arr, 2
        %1 = getelemptr %0, 3
        store 1, %1
        */
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
        // 最后存储右值到目标地址
        koopa_ofs << "\tstore " << exp_result << ", " << CUR_REG_ << endl;
    }
    // 指针类型，即左值是通过函数参数传进来的情况
    else if (symbol.type == Symbol::Type::PTR) {
        /*
        已知 arr 是一个 int[][3] 传进来的指针，即类型为 *[i32, 3]
        那么 arr[2][3] = 1 应当翻译为：
        %0 = load @arr
        %1 = getptr %0, 2
        %2 = getelemptr %1, 3
        store 1, %2
         */
         // 先 load 出基指针
        koopa_ofs << "\t" << NEW_REG_ << " = load @" << ident_with_suffix << endl;
        // 再进行寻址
        for (int i = 0;i < l_val_ast->array_index->size();i++) {
            auto prev_reg = CUR_REG_;
            auto index = (*l_val_ast->array_index)[i]->print();
            // 如果是第一次寻址，则使用 getptr 指令
            if (i == 0) {
                koopa_ofs << "\t" << NEW_REG_ << " = getptr " << prev_reg << ", " << index << endl;
            }
            // 否则使用 getelemptr 指令
            else {
                koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", " << index << endl;
            }
        }
        // 最后存储右值到目标地址
        koopa_ofs << "\tstore " << exp_result << ", " << CUR_REG_ << endl;
    }
    // 若不是数组/指针类型，则直接存储右值到目标地址
    else {
        koopa_ofs << "\tstore " << exp_result << ", @" << ident_with_suffix << endl;
    }
    return Result();
}

/**
 * @brief 打印表达式语句
 * */
Result StmtExpAST::print() const {
    // 若表达式不为空，则计算表达式结果
    // 因为可能有单个 ; 的语句也会生成 StmtExpAST，所以需要判断
    if (exp) {
        return (*exp)->print();
    }
    return Result();
}

/**
 * @brief 打印 return 语句
 * */
Result StmtReturnAST::print() const {
    // 若表达式不为空，则计算表达式结果并返回
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
    // 也即我们总是保证每次生成结束时最后一条语句并非跳转指令，那么就能保证不会出现多条改变控制流的语句都在最后的情况
    auto jump_label = environment_manager.get_jump_label();
    koopa_ofs << jump_label << ":" << endl;
    return Result();
}

/**
 * @brief 打印左值
 * @return 计算结果所在寄存器或立即数
 */
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
    // 若变量是数组类型，则需要进行寻址后处理
    else if (symbol.type == Symbol::Type::ARR) {
        // 准备数组索引
        vector<Result> indices;
        for (auto& item : *array_index) {
            indices.push_back(item->print());
        }
        // ---[DEBUG]---
        // 打印数组索引
        // koopa_ofs << "indices: ";
        // for (auto& item : indices) {
        //   koopa_ofs << item << " ";
        // }
        // koopa_ofs << endl;
        // ---[DEBUG END]---
        // 获取指明的数组维度
        int set_dims = 0;
        // 遍历进行指针寻址
        for (int i = 0;i < indices.size();i++) {
            set_dims += 1;
            auto prev_reg = CUR_REG_;
            // 指针是强类型的，后加 index 会在实际计算时自动乘以当前指针对应类型的步长
            // 特判首次寻址，加 @ 符号
            if (i == 0) {
                koopa_ofs << "\t" << NEW_REG_ << " = getelemptr @" << ident_with_suffix << ", " << indices[i] << endl;
            }
            else {
                koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", " << indices[i] << endl;
            }
        }
        auto prev_reg = CUR_REG_;
        // 如果指明的数组维度与实际数组维度相同，则表明这是加载一个数组元素，使用 load 指令来解引用
        if (set_dims == symbol.value) {
            /*
            int arr[2][3];
            int b = arr[2][3];
            后面 b 的右值则应当翻译为
            %0 = getelemptr @arr, 2
            %1 = getelemptr %0, 3
            %2 = load %1
             */
            koopa_ofs << "\t" << NEW_REG_ << " = load " << prev_reg << endl;
        }
        // 其他情况，都表明这是加载一个指针，然后要用做函数参数传递
        // 此时，应当生成一条 %1 = getelemptr %0, 0 的指令
        // 偏移 0 就是指针本身
        // 如果指明的数组维度为 0，需要特判加 @ 符号
        else if (set_dims == 0) {
            /*
            int arr[2][3];
            f2d(arr);
            准备 arr 作为 f2d 参数时，应当翻译为
            %0 = getelemptr @arr, 0
             */
            koopa_ofs << "\t" << NEW_REG_ << " = getelemptr @" << ident_with_suffix << ", 0" << endl;
        }
        // 其他情况表示我们就是要得到一个指针，直接使用 getelemptr 指令就行
        else {
            /*
            int arr[2][3];
            f2d(arr[1]);
            准备 arr[1] 作为 f2d 参数时，应当翻译为
            %0 = getelemptr @arr, 1
            %1 = getelemptr %0, 0
             */
            koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", 0" << endl;
        }
        return CUR_REG_;
    }
    // 若变量是函数指针类型，也即当前是在函数体内处理传入的参数，则也需要进行寻址后处理
    else if (symbol.type == Symbol::Type::PTR) {
        // 准备数组索引
        vector<Result> indices;
        for (auto& item : *array_index) {
            indices.push_back(item->print());
        }
        // 获取指明的数组维度
        int set_dims = 0;
        // 先 load 出基指针
        koopa_ofs << "\t" << NEW_REG_ << " = load @" << ident_with_suffix << endl;
        // 遍历进行指针寻址
        for (int i = 0;i < indices.size();i++) {
            set_dims += 1;
            auto prev_reg = CUR_REG_;
            // 对指针进行首次寻址，使用 getptr 指令
            if (i == 0) {
                koopa_ofs << "\t" << NEW_REG_ << " = getptr " << prev_reg << ", " << indices[i] << endl;
            }
            // 对指针进行后续寻址，使用 getelemptr 指令
            else {
                koopa_ofs << "\t" << NEW_REG_ << " = getelemptr " << prev_reg << ", " << indices[i] << endl;
            }
        }
        auto prev_reg = CUR_REG_;
        // 如果指明的数组维度与实际数组维度相同，则表明这是加载一个数组元素，使用 load 指令来解引用
        if (set_dims == symbol.value) {
            koopa_ofs << "\t" << NEW_REG_ << " = load " << prev_reg << endl;
        }
        // 其他情况表示我们就是要得到一个指针，直接使用 getelemptr 指令就行
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

/**
 * @brief 打印表达式
 * */
Result ExpAST::print() const {
    return l_or_exp->print();
}

/**
 * @brief 打印逻辑或表达式
 * */
Result LOrExpAST::print() const {
    return l_and_exp->print();
}

/**
 * @brief 打印逻辑与表达式
 * */
Result LAndExpAST::print() const {
    return eq_exp->print();
}

/**
 * @brief 打印逻辑表达式
 * @return 计算结果所在寄存器或立即数
 */
Result LExpWithOpAST::print() const {
    // 先打印计算左表达式结果的语句，并获取左表达式结果
    Result lhs = left->print();
    // 根据逻辑运算符的类型，打印逻辑运算语句
    // 逻辑或运算符
    if (logical_op == LogicalOp::LOGICAL_OR) {
        // 左侧为立即数
        if (lhs.type == Result::Type::IMM) {
            // 如果左侧为立即数，且值不为 0，则直接返回 1，进行短路求值
            if (lhs.value != 0) {
                return IMM_(1);
            }
            // 如果左侧为立即数，且值为 0，则计算右表达式结果
            else {
                Result rhs = right->print();
                // 如果右表达式结果为立即数，则直接返回右表达式结果
                if (rhs.type == Result::Type::IMM) {
                    return IMM_(rhs.value != 0);
                }
                else {
                    // 生成一条 ne 0 指令，相当于 rhs != 0，得到布尔值
                    koopa_ofs << "\t" << NEW_REG_ << " = ne " << rhs << ", 0" << endl;
                    return CUR_REG_;
                }
            }
        }
        // 左侧不为立即数
        auto true_label = environment_manager.get_short_true_label();
        auto false_label = environment_manager.get_short_false_label();
        auto end_label = environment_manager.get_short_end_label();
        auto result = environment_manager.get_short_result_reg();
        environment_manager.add_short_circuit_count();

        // 生成 alloc 指令
        koopa_ofs << "\t" << result << " = alloc i32" << endl;
        environment_manager.is_symbol_allocated[result] = true;

        // 生成 br 指令
        koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;

        // 生成 true 分支
        koopa_ofs << true_label << ":" << endl;
        koopa_ofs << "\t" << "store 1, " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        // 生成 false 分支
        koopa_ofs << false_label << ":" << endl;
        Result rhs = right->print();
        Result temp = NEW_REG_;
        // 生成一条 ne 0 指令，相当于 rhs != 0，得到布尔值
        koopa_ofs << "\t" << temp << " = ne " << rhs << ", 0" << endl;
        koopa_ofs << "\t" << "store " << temp << ", " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        // 生成 end 标签
        koopa_ofs << end_label << ":" << endl;
        Result result_reg = NEW_REG_;
        koopa_ofs << "\t" << result_reg << " = load " << result << endl;

        return result_reg;
    }
    // 逻辑与运算符
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
                    // 生成一条 ne 0 指令，相当于 rhs != 0，得到布尔值
                    koopa_ofs << "\t" << NEW_REG_ << " = ne " << rhs << ", 0" << endl;
                    return CUR_REG_;
                }
            }
        }
        // 左侧不为立即数
        auto true_label = environment_manager.get_short_true_label();
        auto false_label = environment_manager.get_short_false_label();
        auto end_label = environment_manager.get_short_end_label();
        auto result = environment_manager.get_short_result_reg();
        environment_manager.add_short_circuit_count();

        // 生成 alloc 指令
        koopa_ofs << "\t" << result << " = alloc i32" << endl;
        environment_manager.is_symbol_allocated[result] = true;

        // 生成 br 指令
        koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;

        // 生成 false 分支
        koopa_ofs << false_label << ":" << endl;
        koopa_ofs << "\t" << "store 0, " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        // 生成 true 分支
        koopa_ofs << true_label << ":" << endl;
        Result rhs = right->print();
        Result temp = NEW_REG_;
        // 生成一条 ne 0 指令，相当于 rhs != 0，得到布尔值
        koopa_ofs << "\t" << temp << " = ne " << rhs << ", 0" << endl;
        koopa_ofs << "\t" << "store " << temp << ", " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        // 生成 end 标签
        koopa_ofs << end_label << ":" << endl;
        Result result_reg = NEW_REG_;
        koopa_ofs << "\t" << result_reg << " = load " << result << endl;

        return result_reg;
    }
    else {
        assert(false);
    }
}

/**
 * @brief 转换等式运算符，输出枚举类型
 * @param[in] op 等式运算符
 * @return 等式运算符枚举类型
 */
EqExpWithOpAST::EqOp EqExpWithOpAST::convert(const string& op) const {
    if (op == "==") {
        return EqOp::EQ;
    }
    else if (op == "!=") {
        return EqOp::NEQ;
    }
    throw runtime_error("Invalid operator: " + op);
}

/**
 * @brief 打印等式表达式
 * */
Result EqExpAST::print() const {
    return rel_exp->print();
}

/**
 * @brief 打印带符号的等式表达式
 * @return 计算结果所在寄存器或立即数
 */
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
    // 若左右表达式结果不均为常量，则使用临时变量计算结果并存储之
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

/**
 * @brief 打印关系表达式
 * */
Result RelExpAST::print() const {
    return add_exp->print();
}

/**
 * @brief 转换关系运算符，输出枚举类型
 * @param[in] op 关系运算符
 * @return 关系运算符枚举类型
 */
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

/**
 * @brief 打印带符号的关系表达式
 * @return 计算结果所在寄存器或立即数
 */
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
    // 若左右表达式结果不均为常量，则使用临时变量计算结果并存储之
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

/**
 * @brief 打印加法表达式
 * */
Result AddExpAST::print() const {
    return mul_exp->print();
}

/**
 * @brief 转换加法运算符，输出枚举类型
 * @param[in] op 加法运算符
 * @return 加法运算符枚举类型
 */
AddExpWithOpAST::AddOp AddExpWithOpAST::convert(const string& op) const {
    if (op == "+") {
        return AddOp::ADD;
    }
    else if (op == "-") {
        return AddOp::SUB;
    }
    throw runtime_error("Invalid operator: " + op);
}

/**
 * @brief 打印带符号的加法表达式
 * @return 计算结果所在寄存器或立即数
 */
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
    // 若左右表达式结果不均为常量，则使用临时变量计算结果并存储之
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

/**
 * @brief 打印乘法表达式
 * */
Result MulExpAST::print() const {
    return unary_exp->print();
}

/**
 * @brief 转换乘法运算符，输出枚举类型
 * @param[in] op 乘法运算符
 * @return 乘法运算符枚举类型
 */
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

/**
 * @brief 打印带符号的乘法表达式
 * @return 计算结果所在寄存器或立即数
 */
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
    // 若左右表达式结果不均为常量，则使用临时变量计算结果并存储之
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

/**
 * @brief 打印一元表达式
 * */
Result UnaryExpAST::print() const {
    return primary_exp->print();
}

/**
 * @brief 转换一元运算符，输出枚举类型
 * @param[in] op 一元运算符
 * @return 一元运算符枚举类型
 */
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

/**
 * @brief 打印带符号的一元表达式
 * @return 计算结果所在寄存器或立即数
 */
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
    // 若表达式结果为临时变量，则使用临时变量计算结果并存储之
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

/**
 * @brief 打印函数调用一元表达式
 * @return 若函数有返回值，返回结果所在寄存器
 */
Result UnaryExpWithFuncCallAST::print() const {
    // 先计算参数表达式结果
    vector<Result> params;
    for (auto& param : *func_r_params) {
        params.push_back(param->print());
    }
    // 若函数有返回值，则使用临时变量存储结果
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
    // 若函数无返回值，则直接调用函数
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

/**
 * @brief 打印括号优先表达式，即 (a)
 * @return 计算结果所在寄存器或立即数
 */
Result PrimaryExpAST::print() const {
    return exp->print();
}

/**
 * @brief 打印数字优先表达式，即 1
 * @return 立即数
 */
Result PrimaryExpWithNumberAST::print() const {
    return IMM_(number);
}

/**
 * @brief 打印左值优先表达式，即 a
 * @return 左值结果所在寄存器
 */
Result PrimaryExpWithLValAST::print() const {
    return l_val->print();
}