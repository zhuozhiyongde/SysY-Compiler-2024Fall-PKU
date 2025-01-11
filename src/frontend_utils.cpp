#include "include/frontend_utils.hpp"

void SymbolTable::create(const string& ident, Symbol symbol) {
    // 保证当前层级不存在
    assert(symbol_table.find(ident) == symbol_table.end());
    symbol_table[ident] = symbol;
}

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

void SymbolTable::set_parent(SymbolTable* parent) {
    this->parent = parent;
    this->depth = parent->depth + 1;
}

string SymbolTable::locate(const string& ident) {
    string ident_with_suffix = ident + "_" + to_string(depth);
    if (symbol_table.find(ident_with_suffix) == symbol_table.end() && parent) {
        return parent->locate(ident);
    }
    return ident_with_suffix;
}

string SymbolTable::assign(const string& ident) {
    return ident + "_" + to_string(depth);
}

// if-else 语句
string EnvironmentManager::get_then_label() {
    return "%then_" + to_string(if_else_count);
}

string EnvironmentManager::get_else_label() {
    return "%else_" + to_string(if_else_count);
}

string EnvironmentManager::get_end_label() {
    return "%end_" + to_string(if_else_count);
}

// while 语句
string EnvironmentManager::get_while_entry_label(bool current) {
    auto target = current ? while_current : while_count;
    return "%while_entry_" + to_string(target);
}

string EnvironmentManager::get_while_body_label(bool current) {
    auto target = current ? while_current : while_count;
    return "%while_body_" + to_string(target);
}

string EnvironmentManager::get_while_end_label(bool current) {
    auto target = current ? while_current : while_count;
    return "%while_end_" + to_string(target);
}

// 短路求值
string EnvironmentManager::get_short_true_label() {
    return "%short_true_" + to_string(short_circuit_count);
}

string EnvironmentManager::get_short_false_label() {
    return "%short_false_" + to_string(short_circuit_count);
}

string EnvironmentManager::get_short_end_label() {
    return "%short_end_" + to_string(short_circuit_count);
}

string EnvironmentManager::get_short_result_reg() {
    return "%short_result_" + to_string(short_circuit_count);
}

// 跳转语句
string EnvironmentManager::get_jump_label() {
    return "%jump_" + to_string(jump_count++);
}

void EnvironmentManager::add_if_else_count() {
    if_else_count++;
}

void EnvironmentManager::add_while_count() {
    while_count++;
}

void EnvironmentManager::set_while_current(int current) {
    while_current = current;
}

void EnvironmentManager::add_short_circuit_count() {
    short_circuit_count++;
}

int EnvironmentManager::get_temp_count() {
    return temp_count++;
}

int EnvironmentManager::get_while_count() {
    return while_count;
}

int EnvironmentManager::get_while_current() {
    return while_current;
}

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

void print_array_type(const string& ident, const vector<int>& indices) {
    if (environment_manager.is_global) {
        koopa_ofs << "global @" << ident << " = alloc ";
    }
    else {
        koopa_ofs << "\t@" << ident << " = alloc ";
    }
    for (int i = 0;i < indices.size();i++) {
        koopa_ofs << "[";
    }
    koopa_ofs << "i32";
    // 倒序
    for (int i = indices.size() - 1;i >= 0;i--) {
        koopa_ofs << ", " << indices[i] << "]";
    }
}

/**
 * 打印数组初始化值
 * @param indices 数组维度
 * @param values 初始化值
 * @param level 当前维度
 * @param value_index 当前值索引
 */
void print_array(const string& ident, const vector<int>& indices, const vector<int>& values, int level, int& value_index, vector<int>& slices) {
    // 全局数组
    if (environment_manager.is_global) {
        if (value_index == 0) {
            koopa_ofs << ", ";
        }
        if (level == indices.size() - 1) {
            koopa_ofs << "{";
            for (int i = 0;i < indices[level];i++) {
                if (i != 0) {
                    koopa_ofs << ", ";
                }
                if (value_index < values.size()) {
                    koopa_ofs << values[value_index];
                    value_index++;
                }
                else {
                    koopa_ofs << "0";
                }
            }
            koopa_ofs << "}";
        }
        else {
            koopa_ofs << "{";
            for (int i = 0;i < indices[level];i++) {
                if (i != 0) {
                    koopa_ofs << ", ";
                }
                print_array(ident, indices, values, level + 1, value_index, slices);
            }
            koopa_ofs << "}";
        }
    }
    // 局部数组
    else {
        // 特判处理单维数组
        if (indices.size() == 1) {
            if (value_index == 0) {
                koopa_ofs << endl;
            }
            for (int i = 0; i < indices[0]; i++) {
                int item_ptr = environment_manager.get_temp_count();
                koopa_ofs << "\t%" << item_ptr << " = getelemptr @" << ident << ", " << i << endl;
                koopa_ofs << "\tstore " << values[value_index] << ", %" << item_ptr << endl;
                value_index++;
            }
        }
        // 最内层数组
        else if (level == indices.size() - 1) {
            int ptr = slices[level - 1];
            for (int i = 0;i < indices[level];i++) {
                int item_ptr = environment_manager.get_temp_count();
                koopa_ofs << "\t%" << item_ptr << " = getelemptr %" << ptr << ", " << i << endl;
                koopa_ofs << "\tstore " << values[value_index] << ", %" << item_ptr << endl;
                value_index++;
            }
        }
        // 非最内层数组
        else {
            // 顶级数组
            if (level == 0) {
                if (value_index == 0) {
                    koopa_ofs << endl;
                }
                for (int i = 0;i < indices[level];i++) {
                    int base = environment_manager.get_temp_count();
                    koopa_ofs << "\t%" << base << " = getelemptr @" << ident << ", " << 0 << endl;
                    slices.push_back(base);
                    print_array(ident, indices, values, level + 1, value_index, slices);
                    slices.pop_back();
                }
                return;
            }
            // 非顶级数组
            else {
                int step = 1;
                for (int i = level + 1;i < indices.size();i++) {
                    step *= indices[i];
                }
                for (int i = 0; i < indices[level];i++) {
                    int cur = value_index % step;
                    int ptr = environment_manager.get_temp_count();
                    slices.push_back(ptr);
                    koopa_ofs << "\t%" << ptr << " = getelemptr %" << slices[level - 1] << ", " << cur << endl;
                    print_array(ident, indices, values, level + 1, value_index, slices);
                    slices.pop_back();
                }
            }
        }
    }
}
