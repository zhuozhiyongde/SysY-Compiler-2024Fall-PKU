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

## Riscv

首先，我们使用如下代码获取帧栈大小：

```cpp
void visit(const koopa_raw_function_t& func) {
    // 访问所有基本块，bbs: basic block slice
    // 忽略函数名前的@，@main -> main
    riscv_ofs << "\t.globl " << func->name + 1 << endl;
    riscv_ofs << func->name + 1 << ":" << endl;
    int cnt = 0;
    for (size_t i = 0; i < func->bbs.len; ++i) {
        auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        cnt += bb->insts.len;
        for (size_t j = 0; j < bb->insts.len; ++j) {
            auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
            if (inst->ty->tag == KOOPA_RTT_UNIT) {
                cnt -= 1;
            }
        }
    }
    int stack_size = cnt * 4;
};
```

但是，注意到示例代码中，我们实际上是先恢复帧栈再 ret 的，所以我们必须拆开创建帧栈和恢复帧栈到两个地方。

一个合理的想法是把这个帧栈大小置为全局变量，这样就能在过程间共享，但出于可维护性的考量，我们创建一个新的类 `ContextManager` 来管理帧栈，这样即使我们未来需要处理被调用者保存寄存器等问题，也能有比较好的扩展性。

计划创建一个字典，根据函数名来存储栈帧大小。

```cpp
// include/helper.hpp
class Context {
public:
    int stack_size;
    unordered_map<koopa_raw_value_t, string> stack_map;
    Context(int stack_size) : stack_size(stack_size) {}
    void push(const koopa_raw_value_t& value, const string& bias) {
        stack_map[value] = bias + "(sp)";
    }
};

class ContextManager {
public:
    unordered_map<string, Context> context_map;
    void create(const string& name, int stack_size) {
        context_map[name] = Context(stack_size);
    }
    Context& get(const string& name) {
        return context_map[name];
    }
};
```

同时，在 `visit.cpp` 中，创建如下全局变量：

```cpp
ContextManager context_manager;
Context& context; // 当前函数对应的 context
```

我额外重构了所有涉及寄存器分配的部分：

```cpp
// include/helper.hpp
class RegisterManager {
private:
    // 寄存器计数器
    int reg_count = 0;
public:
    // 存储指令到寄存器的映射
    unordered_map<koopa_raw_value_t, string> reg_map;
    string cur_reg() {
        // x0 是一个特殊的寄存器, 它的值恒为 0, 且向它写入的任何数据都会被丢弃.
        // t0 到 t6 寄存器, 以及 a0 到 a7 寄存器可以用来存放临时值.
        if (reg_count < 8) {
            return "t" + to_string(reg_count);
        }
        else {
            return "a" + to_string(reg_count - 8);
        }
    }
    string new_reg() {
        string reg = cur_reg();
        reg_count++;
        return reg;
    }
    // 返回是否需要使用寄存器，主要用于二元表达式存储结果
    // 注意！！这个函数是处理操作数的，而不是指令的
    bool get_operand_reg(const koopa_raw_value_t& value) {
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
            riscv._lw(reg_map[value], context.stack_map[value]);
            return true;
        }
        // 运算数为二元运算的结果，也需要先加载
        // 出现在形如 a = a + b + c 的式子中
        else if (value->kind.tag == KOOPA_RVT_BINARY) {
            reg_map[value] = new_reg();
            riscv._lw(reg_map[value], context.stack_map[value]);
            return true;
        }
        return true;
    }
};

```

这里千万要注意 `KOOPA_RVT_BINARY`，对于一条 `a = a + b + c` 的式子，实际上会被编译为两条二元运算，所以你必须将每次运算的中间结果都压入栈中。

在修改 `visit.cpp` 时，必须需要辨析当前使用的 map 究竟是寄存器的 `reg_map` 还是帧栈的 `stack_map`，它们都以一条代表指令的 `koopa_raw_value_t` 类型为键。

一句话概括就是你得看现在是否在访存，如果在访存，也即 `lw / sw` 指令，那必然需要使用 `stack_map`，如果仅仅是在执行计算，那只需要使用 `reg_map`。

```cpp
// 形如 ret exp, 返回表达式的值
else if (ret.value->kind.tag == KOOPA_RVT_BINARY) {
    riscv._lw("a0", context.stack_map[ret.value]);
    // 或者使用 mv 指令，将寄存器中的值赋值给 a0
    // riscv._mv("a0", register_manager.reg_map[ret.value]);

}
else if (ret.value->kind.tag == KOOPA_RVT_LOAD) {
    riscv._lw("a0", context.stack_map[ret.value]);
}
```

## Debug

发现在实现中忽略了对于立即数偏置的 12 位立即数限制。由于存在交叉引用，遂重构了 `include/helper.hpp`，将实现移动到了 `helper.cpp` 中。

对于 `_sw`、`_lw` 指令，需要检查偏移量是否在 12 位立即数范围内，否则需要使用寄存器来存储偏移量。

新增 `_add_sp` 指令，用于在修改栈指针前预检立即数是否在 12 位立即数范围内，若不在，则使用寄存器来存储偏移量。

又又发现过不去 `21_decl_after_decl3` 测试点，遂检查了一下代码，发现是不能仅仅只在 `load` 、 `store` 指令中重置寄存器计数，对于 `binary` 指令，也需要重置寄存器计数。