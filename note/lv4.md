# Lv4

## Koopa

直接看 lv9 的词法，发现 `BType` 可以直接无缝换为 `int`，遂直接替换。

倒查了一下发现前面的很多实现都是冗余的，尤其是很多对于 optional 的使用，基本上都在 flex 中直接剪枝了。

出于可维护性的考量，我们在这里恢复了部分 AST 类，对于有多个产生式的 AST 类，我们使用 withXXX 的命名来区分产生式体的对应 AST 类。

对于 optional，只供部分可能未初始化的字段使用，如 `VarDefAST` 的 `value` 字段。

为了能够处理变量的定义，我们引入一个新的符号类 `Symbol` 和符号表类 `SymbolTable`，用于存储变量名、变量类型和其对应的符号：

```cpp
class Symbol {
public:
    enum class Type {
        VAR,
        VAL
    };
    Type type;
    int value;
    Symbol() : type(Type::VAL), value(0) {}
    Symbol(Type type, int value = 0) : type(type), value(value) {}
};

class SymbolTable {
private:
    unordered_map<string, Symbol> symbol_table;
    bool is_returned = false;
public:
    void create(const string& name, Symbol symbol);
    bool exist(const string& name);
    Symbol read(const string& name);
    void set_returned(bool is_returned);
    bool get_returned();
};
```

这里额外引入了一个字段 `is_returned`，用于表示当前符号表是否已经返回。这其实是对于测试用例的特殊处理，在测试点 18_multiple_returns2 中，存在多条 return 语句，我们应当只处理到第一个 return 语句后，就停止处理：

```cpp
// ast.cpp 
Result BlockAST::print() const {
  koopa_ofs << "%entry:" << endl;
  for (auto& item : block_items) {
    if (!symbol_table.get_returned()) {
      item->print();
    }
  }
  return Result();
}
```

由于要在生成 koopa 时即完成编译时常量翻译，所以我们需要修改原先的基于 Result 类的 Koopa 输出 print 函数系统。

最纠结的地方在于如何实现这个过程。

由于我们需要实现常量计算，所以我们要思考，何时能够直接在自底向上的翻译过程中即可给出常量？

**答：一个操作数两端都是立即数的时候，我们可以返回一个立即数，若任一为变量，则需要生成计算指令。若一个表达式完全就是一个立即数，其必然可以构建出一个完全由 Result::Type::IMM 组成二叉树。**

所以，只要对这种情况进行特判，就能完成一个自底向上的常量计算。

同时，我们引入部分宏，用于精简代码：

```cpp
#define VAR_ Symbol(Symbol::Type::VAR, 0)
#define VAL_(value) Symbol(Symbol::Type::VAL, value)

#define IMM_(value) Result(Result::Type::IMM, value)
#define REG_(value) Result(Result::Type::REG, value)
```

这里末尾加了一个 `_` 是类比 PyTorch 的“原地操作” 进行命名的，表示“在这里生成一个对应类”，这同时也是为了区分同名类型常量。