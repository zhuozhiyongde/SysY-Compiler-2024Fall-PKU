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

> 需要注意的是，基本块的结尾必须是 br，jump 或 ret 指令其中之一 (并且，这些指令只能出现在基本块的结尾)。也就是说，即使两个基本块是相邻的，例如上述程序的 % else 基本块和 % end 基本块，如果你想表达执行完前者之后执行后者的语义，你也必须在前者基本块的结尾添加一条目标为后者的 jump 指令。这点和汇编语言中 label 的概念有所不同。

值得一提的是，如下 Koopa IR 是合法的：

```asm
%then_0:
	ret 1
%return_end_0:
	jump %end_0
```

所以，我们得到一个弱智但有效的做法：给所有 `ret` 语句后都添加一个新的标签，然后一通加 `jump` 指令，对于 `StmtIfAST`，做如下处理：

```cpp
// ast.cpp
Result StmtIfAST::print() const {
  Result exp_result = exp->print();
  SymbolTable* parent_symbol_table = local_symbol_table;

  string then_label = frontend_context_manager.get_then_label();
  string else_label = frontend_context_manager.get_else_label();
  string end_label = frontend_context_manager.get_end_label();
  frontend_context_manager.add_if_else_count();

  if (else_stmt) {
    koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << else_label << endl;
    koopa_ofs << then_label << ":" << endl;
    then_stmt->print();
    koopa_ofs << "\tjump " << end_label << endl;
    koopa_ofs << else_label << ":" << endl;
    (*else_stmt)->print();
    koopa_ofs << "\tjump " << end_label << endl;
  }
  else {
    koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << end_label << endl;
    koopa_ofs << then_label << ":" << endl;
    then_stmt->print();
    koopa_ofs << "\tjump " << end_label << endl;
  }
  koopa_ofs << end_label << ":" << endl;
  return Result();
}
```

可以看到，我们会大量的生成 `label`，并添加对应的 `jump` 指令，虽然这很冗余，但恰好能完全满足 `%label` 标号块最末必须是 `br`、`jump` 或 `ret` 的要求。

### 处理作用域

考虑一个 `if(0) return 2; return 3;` 的测试点。

如果不给 `MatchedStmt` 新开符号表，那么会导致 `return 2` 影响掉当前符号表的 `is_returned`，从而导致 `return 3` 被跳过。

这里比较烦人的是，我们对于 `BlockAST` 会新建 SymbolTable，但是对于其他语句不会，而 `MatchedStmt` 既可以推导为一个语句，也可以推导为一个 `Block`。

我们必须为单语句的情况也创建一个符号表，用于处理符号重名、是否 return 的判断等。

有两种做法：

1. 在 `StmtIfAST` 中，创建一个临时的 `SymbolTable`，用于避免 `if` 语句中 `return` 影响上层符号表。
  ```cpp
  // ast.cpp
  Result StmtIfAST::print() const {
    Result exp_result = exp->print();
    SymbolTable* parent_symbol_table = local_symbol_table;
    local_symbol_table = new SymbolTable();
    local_symbol_table->set_parent(parent_symbol_table);

    string then_label = frontend_context_manager.get_then_label();
    string else_label = frontend_context_manager.get_else_label();
    string end_label = frontend_context_manager.get_end_label();
    frontend_context_manager.add_if_else_count();

    if (else_stmt) {
      koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << else_label << endl;
      koopa_ofs << then_label << ":" << endl;
      then_stmt->print();
      koopa_ofs << "\tjump " << end_label << endl;
      koopa_ofs << else_label << ":" << endl;
      (*else_stmt)->print();
      koopa_ofs << "\tjump " << end_label << endl;
    }
    else {
      koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << end_label << endl;
      koopa_ofs << then_label << ":" << endl;
      then_stmt->print();
      koopa_ofs << "\tjump " << end_label << endl;
    }
    koopa_ofs << end_label << ":" << endl;
    delete local_symbol_table;
    local_symbol_table = parent_symbol_table;
    return Result();
  }
  ```
2. 在 `sysy.y` 中，直接对于 `MatchedStmt` 创建一个 `BlockAST`，然后：
  1. 对于非 `Block` 的推导，在其 `block_items` 中添加单一的语句。
  2. 对于 `Block` 的推导，直接返回，也即：

    ```bison
    MatchedStmt
      : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
        auto ast = new BlockAST();
        auto block_item = new StmtIfAST();
        block_item->exp = unique_ptr<BaseAST>($3);
        block_item->then_stmt = unique_ptr<BaseAST>($5);
        block_item->else_stmt = unique_ptr<BaseAST>($7);
        ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
        $$ = ast;
      }
      | LVal '=' Exp ';' {
        auto ast = new BlockAST();
        auto block_item = new StmtAssignAST();
        block_item->l_val = unique_ptr<BaseAST>($1);
        block_item->exp = unique_ptr<BaseAST>($3);
        ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
        $$ = ast;
      }
      | Exp ';'{
        auto ast = new BlockAST();
        auto block_item = new StmtExpAST();
        block_item->exp = unique_ptr<BaseAST>($1);
        ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
        $$ = ast;
      }
      | ';'{
        auto ast = new BlockAST();
        auto block_item = new StmtExpAST();
        ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
        $$ = ast;
      }
      | Block {
        $$ = $1;
      }
      | RETURN Exp ';' {
        auto ast = new BlockAST();
        auto block_item = new StmtReturnAST();
        block_item->exp = unique_ptr<BaseAST>($2);
        ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
        $$ = ast;
      }
      | RETURN ';' {
        auto ast = new BlockAST();
        auto block_item = new StmtReturnAST();
        ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
        $$ = ast;
      }
      ;

    OpenStmt
      : IF '(' Exp ')' Stmt {
        auto ast = new BlockAST();
        auto block_item = new StmtIfAST();
        block_item->exp = unique_ptr<BaseAST>($3);
        block_item->then_stmt = unique_ptr<BaseAST>($5);
        ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
        $$ = ast;
      }
      | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
        auto ast = new BlockAST();
        auto block_item = new StmtIfAST();
        block_item->exp = unique_ptr<BaseAST>($3);
        block_item->then_stmt = unique_ptr<BaseAST>($5);
        block_item->else_stmt = unique_ptr<BaseAST>($7);
        ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
        $$ = ast;
      }
      ;
    ```

经过测试，两种方法都可行，但第二种方法会导致 `is_returned` 的判断完全失效，因为 `MatchedStmt` 在所有具体的语句的推导关键路径上，所以任何一条语句都会被包在一个 `BlockAST` 中。

不过，有了对于 `ret` 的标号后处理，我们其实完全不用 care 这件事。这个只影响 `BlockAST` 是否多打印一些语句罢了。

我目前选的是第一种方法。

### 跨 Block 链符号重名

现在对于符号的命名基于 symbol_table 的嵌套深度 `depth`，这会导致跨 Block 链的符号重名时，发生多个同名 `alloc` 指令，这是不合法的。

想要解决这个问题，很简单，只需要维护一个全局的 `is_symbol_allocated` 即可，然后再 `store` 时，检查是否已经分配过，若已经分配过，则不分配，仅重新赋值。

## Riscv

没啥好说的：

```cpp
// visit.cpp
// 
void visit(const koopa_raw_value_t& value) {
  // ...
  switch (kind.tag) {
  // ...
  case KOOPA_RVT_BRANCH:
      // 访问 branch 指令
      visit(kind.data.branch);
      break;
  case KOOPA_RVT_JUMP:
      // 访问 jump 指令
      visit(kind.data.jump);
      break;
  // ...
  }
}

void visit(const koopa_raw_branch_t& branch) {
    // 访问 branch 指令
    register_manager.reset();
    register_manager.get_operand_reg(branch.cond);
    auto cond = register_manager.reg_map[branch.cond];
    riscv._bnez(cond, branch.true_bb->name + 1);
    riscv._beqz(cond, branch.false_bb->name + 1);
}
void visit(const koopa_raw_jump_t& jump) {
    riscv._jump(jump.target->name + 1);
}
```

## Debug

这里发现时钟过不去 `11_logical1` 测试点，先 `AE` 后 `WA`。

首先是 `AE`，发现是我在处理 12 位立即数偏置的时候，错误地使用了 `reg(sp)` 的形式。

实际上偏移量不是指做成 `t1(sp)`，而是先做 `t1 = bias; t1 = sp + t1`，然后再 `lw t0, (t1)`。对 `sw` 指令同理。

接着是 `WA`，发现是我在处理 12 位立即数的时候，寄存器分配出现了问题，我手动调整了 `context.stack_used` 的值为临界值 `2040` 后，发现是我原先对于 `load` 处的寄存器分配有问题，我使用了 `cur_reg` 而不是 `new_reg`，这会导致如果 `load` 指令目标偏置超过 12 位立即数限制，那么在 `riscv._lw(reg, "sp", context.stack_map[load.src]);` 中，会隐式地发现偏置大于 `2048` 并再次分配 `t0` 来存储偏置，从而造成一句 `lw t0, (t0)` 的指令。修改为 `new_reg` 后，即可 AC。

