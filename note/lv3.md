# Lv3

## Lv3.1

### Koopa IR

对于产生式的或 `|` 语法，需要支持。

对于 `.y` 文件：

```bison
UnaryExp
  : PrimaryExp {
    $$ = $1;
  }
  | '+' UnaryExp {
    $$ = $2;
  }
  | '-' UnaryExp {
    $$ = $2;
  }
  | '!' UnaryExp {
    $$ = $2;
  }
  ;
```

对于 AST，由于所有的 AST 都继承自 `BaseAST`，所以我们其实可以使用 `unique_ptr<BaseAST>` 来表示一个 AST 节点，无论他的子节点是基于 `UnaryOp UnaryExp` 得到的还是 `PrimaryExp` 得到的。

但是，出于维护性考虑，我们在这里改变之前的成员类型写法 `unique_ptr<BaseAST>`，使用更严苛的成员类型写法，即要求尖括号 `<>` 中填入具体的类型。

同时，我们引入 `optional` 类型，来表示一个 AST 节点可能不存在的情况。

```cpp
// include/ast.hpp
#include <optional>
// ...
class UnaryExpAST : public BaseAST {
public:
    optional<unique_ptr<BaseAST>> primary_exp;
    optional<unique_ptr<UnaryExpWithOpAST>> unary_exp_with_op;
    void print() const override;
};
```

这样，在 `ast.cpp` 中，我们需要对 `optional` 类型进行一步判断后解引用：

```cpp
// ast.cpp
void UnaryExpAST::print() const {
  if (primary_exp) {
    (*primary_exp)->print();
  }
  else if (unary_exp_with_op) {
    (*unary_exp_with_op)->print();
  }
}
```

由于我们不需要向前兼容，所以移除掉 `ast.cpp` 中对于 mode 的判断。

由于我们现在的 Return 结果不再是唯一的数字类型了，所以我们需要在 `include/ast.hpp` 中引入 `Result` 类，来表示一个返回值：

```cpp
// include/ast.hpp
class Result {
public:
    enum class Type {
        IMM,
        REG
    };
    Type type;
    int val;
    friend ostream& operator<<(ostream& os, const Result& result) {
        os << (result.type == Type::REG ? "%" : "") << result.val;
        return os;
    }
    Result() : type(Type::IMM), val(0) {}
    Result(Type type, int val) : type(type), val(val) {}
};
```

这里重载了 `<<` 运算符，用于输出 `Result` 类型的值。

我们修改所有的 `print()` 函数，使之返回 `Result` 类型，这样的话，我们在嵌套 AST 各个层级时，就能拿到下级 AST 的返回值，并据此操作。

对于不需要 Result 返回值的 AST 类，我们返回一个空的 `Result` 对象即可：

```cpp
Result FuncDefAST::print() const {
  koopa_ofs << "fun @" << ident << "(): ";
  func_type->print();
  koopa_ofs << " {" << endl;
  block->print();
  koopa_ofs << "}" << endl;
  return Result();
}
```

对于只有一个产生式体的 AST 类，我们可以直接返回其产生式体的 Result 值：

```cpp
Result ExpAST::print() const {
  return unary_exp->print();
}
```

### RISC-V

首先，我们需要修改：

```cpp
void visit(const koopa_raw_value_t& value) {
    // 根据指令类型判断后续需要如何访问
    switch (kind.tag) {
      // ...
    }
}
```

这里，添加一个分支，处理新获得的 `KOOPA_RVT_BINARY` 类型指令。

查看它的类型定义：

```cpp
typedef struct {
  koopa_raw_binary_op_t op;
  koopa_raw_value_t lhs;
  koopa_raw_value_t rhs;
} koopa_raw_binary_t;
```

从而知道，`koopa_raw_binary_t` 类型指令的左右操作数分别是 `lhs` 和 `rhs`，操作符是 `op`。

这里，为了方便 debug，我们引入 `include/utils.hpp` 以及对应的 `utils.cpp` 文件，创建辅助函数（具体实现见源文件，就是无聊的 switch case）：

```cpp
#pragma once
#include "koopa.h"
#include <string>

std::string koopaRawValueTagToString(int tag);
std::string koopaRawBinaryOpToString(int op);
```

那这就会带来一个问题：

怎么确定一个操作数是立即数，还是一个之前指令的结果？

答案是，根据 `tag` 来判断：

- 如果 `tag` 是 `KOOPA_RVT_INTEGER`，那么他就是立即数。
- 如果 `tag` 是 `KOOPA_RVT_BINARY`，那么他就是之前某条指令的结果。

所以，为了让我们的 visit 函数能正确处理这里的寄存器，我们需要在处理 `visit(const koopa_raw_binary_t& binary)` 时，递归处理 `lhs` 和 `rhs`，且处理时，我们就需要设法管理寄存器，从而能在前面存、在后面取。

指令类型通过 `kind.tag` 来区分，指令内容通过 `kind.data` 来访问。

由于我们需要存储一条指令的返回结果（以供未来使用），所以我们还需要修改这个 visit 函数，引入第二个参数 `value`，来表示当前指令：

```cpp
void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
    // ...
}
```

同时，我们还要处理 ret 语句的 visit 函数，使之支持返回表达式的值。

形如 `ret %1` 的指令，其 `ret.value` 就是之前的指令的指针（就是文档的说法）。

```cpp
void visit(const koopa_raw_return_t& ret) {
    // 形如 ret 1 直接返回整数的
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER) {
        riscv_ofs << "\tli a0, ";
        visit(ret.value->kind.data.integer);
        riscv_ofs << endl;
        riscv_ofs << "\tret" << endl;
    }
    // 形如 ret exp, 返回表达式的值
    else if (ret.value->kind.tag == KOOPA_RVT_BINARY) {
        riscv_ofs << "\tmv a0, " << symbol_map[ret.value] << endl;
        riscv_ofs << "\tret" << endl;
    }
    else {
        riscv_ofs << "\tli a0, 0" << endl;
        riscv_ofs << "\tret" << endl;
    }
};
```

## Lv3.2

### Koopa IR

没啥好说的，和 Lv 3.1 思路类似的修改方式。

### RISC-V

