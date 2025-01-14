# Lv3

## Lv3.1

### Koopa

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

由于我们不需要向前兼容那个字符形式的 AST 输出，所以移除掉 `ast.cpp` 中对于 mode 的判断。

由于我们现在的 Return 结果不再是唯一的数字类型了，所以我们需要在 `include/frontend_utils.hpp` 中引入 `Result` 类，来表示一个返回值：

```cpp
// include/frontend_utils.hpp
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

对于不需要 `Result` 返回值的 AST 类，我们返回一个空的 `Result` 对象即可：

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

### Riscv

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

这里，为了方便 debug，我们引入 `include/other_utils.hpp` 以及对应的 `other_utils.cpp` 文件，创建辅助函数（具体实现见源文件，就是无聊的 `switch` `case`）：

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

所以，为了让我们的 `visit` 函数能正确处理这里的寄存器，我们需要在处理 `visit(const koopa_raw_binary_t& binary)` 时，递归处理 `lhs` 和 `rhs`，且处理时，我们就需要设法管理寄存器，从而能在前面存、在后面取。

指令类型通过 `kind.tag` 来区分，指令内容通过 `kind.data` 来访问。

由于我们需要存储一条指令的返回结果（以供未来使用），所以我们还需要修改这个 `visit` 函数，引入第二个参数 `value`，来表示当前指令：

```cpp
void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
    // ...
}
```

同时，我们还要处理 `ret` 语句的 `visit` 函数，使之支持返回表达式的值。

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

### Koopa

没啥好说的，和 Lv 3.1 思路类似的修改方式。

### Riscv

同上。

## Lv3.3

### Koopa

草，早知道直接把 Lv3.2 和 Lv3.3 写一起了，这改来改去的，烦死了。

添加多字符的标识符有两种方式：

第一种，修改 `sysy.l` 文件：

```flex
"int"           { return INT; }
"return"        { return RETURN; }
"<="            { return LE; }
">="            { return GE; }
"=="            { return EQ; }
"!="            { return NEQ; }
"&&"            { return LOGICAL_AND; }
"||"            { return LOGICAL_OR; }
```

第二种，在 `sysy.y` 文件中，使用多个 `''` 来读入：

```bison
RelExp '<' '=' AddExp
```

我选择的是第一种。

翻译逻辑与、逻辑或为二元表达式：

```cpp
Result LExpWithOpAST::print() const {
  Result left_result = left->print();
  Result right_result = right->print();
  Result result = Result(Result::Type::REG, TEMP_COUNT++);
  if (logical_op == LogicalOp::LOGICAL_OR) {
    Result temp = Result(Result::Type::REG, TEMP_COUNT++);
    koopa_ofs << "\t" << temp << " = or " << left_result << ", " << right_result << endl;
    koopa_ofs << "\t" << result << " = ne " << temp << ", 0" << endl;
  }
  else if (logical_op == LogicalOp::LOGICAL_AND) {
    Result temp_1 = Result(Result::Type::REG, TEMP_COUNT++);
    Result temp_2 = Result(Result::Type::REG, TEMP_COUNT++);
    koopa_ofs << "\t" << temp_1 << " = ne " << left_result << ", 0" << endl;
    koopa_ofs << "\t" << temp_2 << " = ne " << right_result << ", 0" << endl;
    koopa_ofs << "\t" << result << " = and " << temp_1 << ", " << temp_2 << endl;
  }

  return result;
}
```

### Riscv

新增 Koopa 指令：

```cpp
KOOPA_RBO_NOT_EQ
KOOPA_RBO_LE
KOOPA_RBO_GE
KOOPA_RBO_LT
KOOPA_RBO_GT
KOOPA_RBO_OR
KOOPA_RBO_AND
```

和先前一样，我们在 `visit.cpp` 中的下述函数签名处，添加对新指令的处理：

```cpp
void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value);
```

我们可以和文档一样，在 [Godbolt](https://godbolt.org/) 上查看正确的代码是怎么编译的。

- 左边选择 `C`
- 右边选择 `RISC-V rv32gc clang(trunk)`

发现 `27_complex_binary` 寄存器超过使用限制了，看来得复用寄存器才行，不能每个中间结果都开一个新的。

正好重新设计一下 Riscv 指令的实现，新建一个 `include/backend_utils.hpp`，在其中实现一个`Riscv` 类，来专门处理 Riscv 指令。

本节中有两个小的细节。

首先是如何确定 reg 来存储中间结果：

```cpp
bool is_use_reg(const koopa_raw_value_t& value) {
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
        if (value->kind.data.integer.value == 0) {
            symbol_map[value] = "x0";
            // 只有 0 的话，不需要占用新的寄存器
            return false;
        }
        else {
            // 说明是个立即数，需要加载到寄存器中
            auto reg = get_reg();
            riscv._li(reg, value->kind.data.integer.value);
            symbol_map[value] = reg;
            reg_count++;
            // 由于存在一个中间结果，会占用一个寄存器
            return true;
        }
    }
    // 对于中间结果，会占用一个寄存器
    return true;
}

void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
    // 访问 binary 指令
    bool lhs_use_reg = is_use_reg(binary.lhs);
    bool rhs_use_reg = is_use_reg(binary.rhs);

    // 确定中间结果的寄存器
    // 如果两个都是整数，显然要新开一个寄存器，来存储中间结果
    if (!lhs_use_reg && !rhs_use_reg) {
        symbol_map[value] = get_reg();
        reg_count++;
    }
    // 对于其他情况，找一个已有寄存器来存储中间结果
    else if (lhs_use_reg) {
        symbol_map[value] = symbol_map[binary.lhs];
    }
    else {
        symbol_map[value] = symbol_map[binary.rhs];
    }
    // ...
}
```

可以看到，选择策略为，若两边都是 0，则需要新开一个寄存器，否则，选择已有寄存器。

第二个细节是，如何处理 RISC-V 中没有的 LE / GE 指令:

```cpp
case KOOPA_RBO_LE:
    // lhs <= rhs 等价于 !(lhs > rhs)
    riscv._sgt(cur, lhs, rhs);
    riscv._seqz(cur, cur);
    break;
case KOOPA_RBO_GE:
    // lhs >= rhs 等价于 !(lhs < rhs)
    riscv._slt(cur, lhs, rhs);
    riscv._seqz(cur, cur);
    break;
```

答案就是使用等价替换。

## Extra

在上述实现中，不难发现对于 op 的使用过于繁琐，我们找一个办法来解耦代码。以下给出一个示例。

将

```bison
"<="            { return LE; }
">="            { return GE; }
```

改为

```bison
RelOp         ("<"|">"|"<="|">=")
```

将

```flex

RelExp
  : AddExp {
    $$ = $1;
  }
  | RelExp LE AddExp {
    auto ast = new RelExpWithOpAST();
    ast->rel_op = RelExpWithOpAST::RelOp::LE;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GE AddExp {
    auto ast = new RelExpWithOpAST();
    ast->rel_op = RelExpWithOpAST::RelOp::GE;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpWithOpAST();
    ast->rel_op = RelExpWithOpAST::RelOp::LT;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpWithOpAST();
    ast->rel_op = RelExpWithOpAST::RelOp::GT;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;
```

改为

```flex
RelExp
  : AddExp {
    $$ = $1;
  }
  | RelExp RelOp AddExp {
    auto ast = new RelExpWithOpAST();
    auto rel_op = *unique_ptr<string>($2);
    ast->rel_op = ast->convert(rel_op);
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;
```

同时实现对应的 `RelExpWithOpAST::convert` 函数：

```cpp
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
```

注意，这里有一个小细节问题，由于二元运算符 `+` / `-` 亦可作为单目运算符，所以被迫在不同地方复用：

```flex
AddExp
  : MulExp {
    $$ = $1;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpWithOpAST();
    auto add_op = *unique_ptr<string>($2);
    ast->add_op = ast->convert(add_op);
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddOp UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    auto add_op = *unique_ptr<string>($1);
    ast->unary_op = ast->convert(add_op);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | NotOp UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    auto not_op = *unique_ptr<string>($1);
    ast->unary_op = ast->convert(not_op);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
```

## Debug

Lv3 会有一个测试点过不了，但无所谓，Lv4 会将寄存器完全改为使用栈来存储，所以不用 care，写完 Lv4 自然就过了。
