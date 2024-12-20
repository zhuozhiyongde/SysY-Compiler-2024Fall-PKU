#include "include/type.hpp"

void SymbolTable::create(const string& name, Symbol symbol) {
    symbol_table[name] = symbol;
}

Symbol SymbolTable::read(const string& name) {
    return symbol_table[name];
}

bool SymbolTable::exist(const string& name) {
    return symbol_table.find(name) != symbol_table.end();
}

void SymbolTable::set_returned(bool is_returned) {
    this->is_returned = is_returned;
}

bool SymbolTable::get_returned() {
    return this->is_returned;
}