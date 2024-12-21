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

string FrontendContextManager::get_then_label() {
    return "%then_" + to_string(if_else_count);
}

string FrontendContextManager::get_else_label() {
    return "%else_" + to_string(if_else_count);
}

string FrontendContextManager::get_end_label() {
    return "%end_" + to_string(if_else_count);
}

string FrontendContextManager::get_ret_end_label() {
    return "%return_end_" + to_string(ret_count);
}

void FrontendContextManager::add_if_else_count() {
    if_else_count++;
}

void FrontendContextManager::add_ret_count() {
    ret_count++;
}