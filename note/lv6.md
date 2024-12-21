# Lv6

## Koopa

### 回忆：dangling-else

$$
\begin{aligned}
\textit{stmt} \rightarrow& \textbf{if} \ \textit{expr} \ \textbf{then} \ \textit{stmt} \\
|& \textbf{if} \ \textit{expr} \ \textbf{then} \ \textit{stmt} \ \textbf{else} \ \textit{stmt} \\
|& \textit{other}
\end{aligned}
$$

在这个语法下，$\textbf{if} \ \textit{expr}_1 \ \textbf{then} \ \textbf{if} \ \textit{expr}_2 \ \textbf{then} \ \textit{stmt}_1 \ \textbf{else} \ \textit{stmt}_2$ 有两棵语法树：

![image-20241109131909671](https://cdn.arthals.ink/bed/2024/11/image-20241109131909671-1cdef2a5c1683ed89b87889703d49646.png)

即：这个 else 既可以和第一个 then 匹配，也可以和第二个 then 匹配。

#### 消除 dangling-else 二义性

引入 `matched_stmt` 表示匹配好的语句，文法如下：

$$
\begin{aligned}
\textit{stmt} \rightarrow& \textit{matched\_stmt} | \textit{open\_stmt} \\
\textit{matched\_stmt} \rightarrow& \textbf{if} \ \textit{expr} \ \textbf{then} \ \textit{matched\_stmt} \ \textbf{else} \ \textit{matched\_stmt} \\
|& \textit{other} \\
\textit{open\_stmt} \rightarrow& \textbf{if} \ \textit{expr} \ \textbf{then} \ \textit{stmt} \\
|& \textbf{if} \ \textit{expr} \ \textbf{then} \ \textit{matched\_stmt} \ \textbf{else} \ \textit{open\_stmt}
\end{aligned}
$$

即：通过引入新的非终结符，来保证 else 与最近未匹配的 then 匹配。

我们实现一下：

```bison
<!-- sysy.y -->
Stmt
  : MatchedStmt {
    $$ = $1;
  }
  | OpenStmt {
    $$ = $1;
  }
  ;

MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAssignAST();
    ast->l_val = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';'{
    auto ast = new StmtExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';'{
    auto ast = new StmtExpAST();
    $$ = ast;
  }
  | Block {
    auto ast = new StmtBlockAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtReturnAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtReturnAST();
    $$ = ast;
  }
  ;

OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;
```

```cpp
// include/ast.hpp
class StmtIfAST : public BaseAST {
public:
    unique_ptr<BaseAST> exp;
    unique_ptr<BaseAST> then_stmt;
    optional<unique_ptr<BaseAST>> else_stmt;
    Result print() const override;
};
```

### 处理最后一条语句

这里比较烦人的是，我们对于 `BlockAST` 会新建 SymbolTable，但是对于其他语句不会，而 `MatchedStmt` 既可以推导为一个语句，也可以推导为一个 `Block`。

我们必须为单语句的情况也创建一个符号表，用于处理符号重名、是否 return 的判断等。

有两种做法：

1. 在 `class SymbolTable` 中，添加一个 `is_child_returned` 表示子符号表是否已经返回。在 `ast.cpp` 中，使用 `typeid` 判断 `stmt` 是否为 `StmtBlockAST`，若是，则不必额外创建符号表，若否，则创建符号表，并设置 `parent` 关系。注意，若选用此做法，你需要在 `if` 语句处理 `then` 和 `else` 之间，重置 `is_child_returned` 为 `false`。
2. 在 `sysy.y` 中，直接对于 `MatchedStmt` 创建一个 `BlockAST`，移除 ` StmtBlockAST`，然后：
  1. 对于非 `Block` 的推导，在其 `block_items` 中添加单一的语句。
  2. 对于 `Block` 的推导，直接返回。

我这里暂时选用的是第一种做法，可能稍后会重构为第二种，因为第一种还依赖于 `typeid`，感觉比较烦。

### 跨 Block 链符号重名

现在对于符号的命名基于 symbol_table 的嵌套深度 `depth`，这会导致跨 Block 链的符号重名时，发生多个同名 `alloc` 指令，这是不合法的。

想要解决这个问题，很简单，只需要维护一个全局的 `is_symbol_allocated` 即可，然后再 `store` 时，检查是否已经分配过，若已经分配过，则不分配，仅重新赋值。

