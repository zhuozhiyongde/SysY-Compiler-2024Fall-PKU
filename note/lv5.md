# Lv5

## Koopa

由于我们要实现多级 Block 块，所以我们要为每个 Block 块分配一个符号表。

各级符号表显然随着 Block 块的嵌套而嵌套，这自然而然地形成了一个链表的结构。

根据这个思路，我们首先修改 `include/frontend_utils.hpp`，增加两个成员变量 `parent` 和 `depth`，分别用于表示指向父符号表的指针和当前符号表的深度，后者用于在 Koopa 中间代码中，区分不同层级的同名变量，并添加相应的成员函数。

```cpp
// include/type.hpp
class SymbolTable {
private:
    unordered_map<string, Symbol> symbol_table;
    SymbolTable* parent = nullptr;
    int depth = 0;
    bool is_returned = false;
public:
    void create(const string& ident, Symbol symbol);
    bool exist(const string& ident);
    Symbol read(const string& ident);
    void set_parent(SymbolTable* parent);
    void set_depth(int depth);
    // 从当前层符号表中递归向上查找变量名
    string locate(const string& ident);
    // 在当前层符号表中分配变量名
    string assign(const string& ident);
    void set_returned(bool is_returned);
    bool get_returned();
};
```

然后，我们修改 `frontend_utils.cpp`，实现这些成员函数：

```cpp
// frontend_utils.cpp
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

void SymbolTable::set_depth(int depth) {
    this->depth = depth;
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
```

注意区分 `locate` 和 `assign` 两个函数：

- 前者用于从当前层符号表中递归向上查找变量名
- 后者用于在当前层符号表中分配变量名。

这两个函数在 `ast.cpp` 中区分定义和使用，定义时使用 `assign`，使用时使用 `locate`。

## Riscv

没有修改。

## Debug

Lv5 存在标号块最后一条语句的问题，请参阅 `Lv6 - Koopa` 中的修复。