# Lv3

## Lv3.1

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

