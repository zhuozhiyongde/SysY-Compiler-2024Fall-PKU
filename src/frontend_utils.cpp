#include "include/frontend_utils.hpp"

/**
 * @brief 创建符号表，向符号表中添加符号
 * @param[in] ident 符号名
 * @param[in] symbol 符号
 */
void SymbolTable::create(const string& ident, Symbol symbol) {
    // 保证当前层级不存在
    assert(symbol_table.find(ident) == symbol_table.end());
    symbol_table[ident] = symbol;
}

/**
 * @brief 读取符号表，从符号表中读取符号，若当前层级不存在，则向父级符号表中查找
 * @param[in] ident 符号名
 * @return 符号
 */
Symbol SymbolTable::read(const string& ident) {
    if (symbol_table.find(ident) != symbol_table.end()) {
        return symbol_table[ident];
    }
    else if (parent) {
        return parent->read(ident);
    }
    else {
        return Symbol();
    }
}

/**
 * @brief 判断符号是否存在，若当前层级不存在，则向父级符号表中查找
 * @param[in] ident 符号名
 * @return 符号是否存在
 */
bool SymbolTable::exist(const string& ident) {
    if (symbol_table.find(ident) != symbol_table.end()) {
        return true;
    }
    else if (parent) {
        return parent->exist(ident);
    }
    else {
        return false;
    }
}

/**
 * @brief 设置父级符号表，并设置当前符号表的深度为父级符号表的深度加 1
 * @param[in] parent 父级符号表的指针
 */
void SymbolTable::set_parent(SymbolTable* parent) {
    this->parent = parent;
    this->depth = parent->depth + 1;
}

/**
 * @brief 定位符号，若当前层级不存在，则向父级符号表中查找
 * @param[in] ident 符号名
 * @return 符号名
 */
string SymbolTable::locate(const string& ident) {
    string ident_with_suffix = ident + "_" + to_string(depth);
    if (symbol_table.find(ident_with_suffix) == symbol_table.end() && parent) {
        return parent->locate(ident);
    }
    return ident_with_suffix;
}

/**
 * @brief 分配符号，在当前符号表中为符号分配一个唯一的标识符，但尚未在符号表中创建
 * @param[in] ident 符号名
 * @return 符号名
 */
string SymbolTable::assign(const string& ident) {
    return ident + "_" + to_string(depth);
}

/**
 * @brief 生成 if-else 语句的 then 基本块标签
 * @return 标签
 */
string EnvironmentManager::get_then_label() {
    return "%then_" + to_string(if_else_count);
}

/**
 * @brief 生成 if-else 语句的 else 基本块标签
 * @return 标签
 */
string EnvironmentManager::get_else_label() {
    return "%else_" + to_string(if_else_count);
}

/**
 * @brief 生成 if-else 语句的 end 基本块标签
 * @return 标签
 */
string EnvironmentManager::get_end_label() {
    return "%end_" + to_string(if_else_count);
}

/**
 * @brief 生成 while 语句的 entry 基本块标签
 * @param[in] current 是否为当前 while 语句，控制是在生成 while 语句块处使用还是 continue 语句处使用
 * @return 标签
 */
string EnvironmentManager::get_while_entry_label(bool current) {
    auto target = current ? while_current : while_count;
    return "%while_entry_" + to_string(target);
}

/**
 * @brief 生成 while 语句的 body 函数体基本块标签
 * @return 标签
 */
string EnvironmentManager::get_while_body_label() {
    return "%while_body_" + to_string(while_count);
}

/**
 * @brief 生成 while 语句的 end 基本块标签
 * @param[in] current 是否为当前 while 语句，控制是在生成 while 语句块处使用还是 break 语句处使用
 * @return 标签
 */
string EnvironmentManager::get_while_end_label(bool current) {
    auto target = current ? while_current : while_count;
    return "%while_end_" + to_string(target);
}

/**
 * @brief 生成短路求值的 true 基本块标签
 * @return 标签
 */
string EnvironmentManager::get_short_true_label() {
    return "%short_true_" + to_string(short_circuit_count);
}

/**
 * @brief 生成短路求值的 false 基本块标签
 * @return 标签
 */
string EnvironmentManager::get_short_false_label() {
    return "%short_false_" + to_string(short_circuit_count);
}

/**
 * @brief 生成短路求值的 end 基本块标签
 * @return 标签
 */
string EnvironmentManager::get_short_end_label() {
    return "%short_end_" + to_string(short_circuit_count);
}

/**
 * @brief 生成短路求值的 result 寄存器
 * @return 寄存器
 */
string EnvironmentManager::get_short_result_reg() {
    return "%short_result_" + to_string(short_circuit_count);
}

/**
 * @brief 生成跳转语句的标签，并会增加跳转语句计数器
 * @return 标签
 */
string EnvironmentManager::get_jump_label() {
    return "%jump_" + to_string(jump_count++);
}

/**
 * @brief 增加 if-else 语句计数器
 */
void EnvironmentManager::add_if_else_count() {
    if_else_count++;
}

/**
 * @brief 增加 while 语句计数器
 */
void EnvironmentManager::add_while_count() {
    while_count++;
}

/**
 * @brief 设置当前 while 语句计数器
 * @param[in] current 当前 while 语句计数器
 */
void EnvironmentManager::set_while_current(int current) {
    while_current = current;
}

/**
 * @brief 增加短路求值计数器
 */
void EnvironmentManager::add_short_circuit_count() {
    short_circuit_count++;
}

/**
 * @brief 增加临时变量计数器
 * @return 临时变量计数器
 */
int EnvironmentManager::get_temp_count() {
    return temp_count++;
}

/**
 * @brief 获取 while 语句计数器
 * @return while 语句计数器
 */
int EnvironmentManager::get_while_count() {
    return while_count;
}

/**
 * @brief 获取当前 while 语句计数器
 * @return 当前 while 语句计数器
 */
int EnvironmentManager::get_while_current() {
    return while_current;
}

/**
 * @brief 初始化库函数声明
 */
void init_lib() {
    koopa_ofs << "decl @getint(): i32" << endl;
    koopa_ofs << "decl @getch(): i32" << endl;
    koopa_ofs << "decl @getarray(*i32): i32" << endl;
    environment_manager.is_func_return["getint"] = true;
    environment_manager.is_func_return["getch"] = true;
    environment_manager.is_func_return["getarray"] = true;

    koopa_ofs << "decl @putint(i32)" << endl;
    koopa_ofs << "decl @putch(i32)" << endl;
    koopa_ofs << "decl @putarray(i32, *i32)" << endl;

    koopa_ofs << "decl @starttime()" << endl;
    koopa_ofs << "decl @stoptime()" << endl;
    koopa_ofs << endl;
}

/**
 * @brief 格式化数组类型，输入 <2,3>，输出 [[i32, 3], 2]
 * @param indices 数组维度向量，正序
 * @note 对于空向量，输出 i32
 */
void format_array_type(const vector<int>& indices) {
    if (indices.empty()) {
        koopa_ofs << "i32";
    }
    else {
        for (int i = 0;i < indices.size();i++) {
            koopa_ofs << "[";
        }
        koopa_ofs << "i32";
        // 倒序
        for (int i = indices.size() - 1;i >= 0;i--) {
            koopa_ofs << ", " << indices[i] << "]";
        }
    }
}

/**
 * @brief 打印数组类型声明，如 @arr = alloc [[i32, 3], 2]
 * @param[in] ident 数组名
 * @param[in] indices 数组维度
 * @note 会在其内判断是否为全局数组，并前加 global
 */
void print_array_type(const string& ident, const vector<int>& indices) {
    if (environment_manager.is_global) {
        koopa_ofs << "global @" << ident << " = alloc ";
    }
    else {
        koopa_ofs << "\t@" << ident << " = alloc ";
    }
    format_array_type(indices);
}

/**
 * @brief 打印数组初始化值 aggregate，如 {{1, 2, 3}, {4, 5, 6}}
 * @param[in] ident 数组名
 * @param[in] indices 数组维度
 * @param[in] array 初始化值
 * @param[in] level 当前维度
 * @param[in] index 当前值索引
 * @param[in] bases 基址
 */
void print_array(const string& ident, const vector<int>& indices, int* array, int level, int& index, vector<int>& bases) {
    // 全局数组
    if (environment_manager.is_global) {
        // 初始条件特判
        if (index == 0 && level == 0) {
            koopa_ofs << ", ";
        }
        // 最内层数组
        if (level == indices.size() - 1) {
            koopa_ofs << "{";
            for (int i = 0;i < indices[level];i++) {
                if (i != 0) {
                    koopa_ofs << ", ";
                }
                koopa_ofs << array[index];
                index++;
            }
            koopa_ofs << "}";
        }
        // 非最内层数组，嵌套 aggregate
        else {
            koopa_ofs << "{";
            for (int i = 0;i < indices[level];i++) {
                if (i != 0) {
                    koopa_ofs << ", ";
                }
                print_array(ident, indices, array, level + 1, index, bases);
            }
            koopa_ofs << "}";
        }
    }
    // 局部数组
    else {
        // 初始条件特判
        if (index == 0 && level == 0) {
            koopa_ofs << endl;
        }
        // 最内层数组
        if (level == indices.size() - 1) {
            int base = bases.back();
            for (int i = 0;i < indices[level];i++) {
                int ptr = environment_manager.get_temp_count();
                if (base == -1) {
                    koopa_ofs << "\t%" << ptr << " = getelemptr @" << ident << ", " << i << endl;
                }
                else {
                    koopa_ofs << "\t%" << ptr << " = getelemptr %" << base << ", " << i << endl;
                }
                koopa_ofs << "\tstore " << array[index] << ", %" << ptr << endl;
                index++;
            }
        }
        // 非最内层数组，嵌套 aggregate
        else {
            int base = bases.back();
            for (int i = 0;i < indices[level];i++) {
                int ptr = environment_manager.get_temp_count();
                if (base == -1) {
                    koopa_ofs << "\t%" << ptr << " = getelemptr @" << ident << ", " << i << endl;
                }
                else {
                    koopa_ofs << "\t%" << ptr << " = getelemptr %" << base << ", " << i << endl;
                }
                bases.push_back(ptr);
                print_array(ident, indices, array, level + 1, index, bases);
                bases.pop_back();
            }
        }
    }
}
