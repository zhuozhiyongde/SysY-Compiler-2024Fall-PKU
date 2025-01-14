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
Stmt
  : MatchedStmt {
    // if-else 完全匹配的语句，保证 else 永远匹配到最近的 if
    $$ = $1;
  }
  | OpenStmt {
    // if-else 不匹配的语句
    $$ = $1;
  }
  ;

MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    // if-else 语句，保证 else 永远匹配到最近的 if
    // 注意，这可能会嵌套定义。
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    // 赋值语句，如 a = 1;
    auto ast = new StmtAssignAST();
    ast->l_val = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';'{
    // 表达式语句，如 a + 1;
    auto ast = new StmtExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';'{
    // 空语句，如 ;
    auto ast = new StmtExpAST();
    $$ = ast;
  }
  | Block {
    // 块语句，如 { int a = 1; }
    $$ = $1;
  }
  | RETURN Exp ';' {
    // 带返回值的 return 语句，如 return 1;
    auto ast = new StmtReturnAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    // 不带返回值的 return 语句，如 return;
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
    // 保证 else 永远匹配到最近的 if
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

所以，我们得到一个弱智但有效的做法：给所有 `ret` 语句后都添加一个新的标签，保证每个 `print` 函数的末尾不是一条 `br` / `jump` / `ret`，就可以了。

然后再在 `if` 语句处理里就可以随便加 `jump` 指令了，对于 `StmtIfAST`，做如下处理：

```cpp
/**
 * @brief 打印 if 语句
 * */
Result StmtIfAST::print() const {
    // 先打印条件表达式，并存储计算得出的条件表达式结果
    Result exp_result = exp->print();
    // 准备标签
    string then_label = environment_manager.get_then_label();
    string else_label = environment_manager.get_else_label();
    string end_label = environment_manager.get_end_label();
    environment_manager.add_if_else_count();
    // 根据是否存在 else 语句进行分支处理
    if (else_stmt) {
        // 生成 br 指令
        koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << else_label << endl;
        // 生成 then 语句块
        koopa_ofs << then_label << ":" << endl;
        then_stmt->print();
        koopa_ofs << "\tjump " << end_label << endl;
        // 生成 else 语句块
        koopa_ofs << else_label << ":" << endl;
        (*else_stmt)->print();
        koopa_ofs << "\tjump " << end_label << endl;
    }
    else {
        // 生成 br 指令
        koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << end_label << endl;
        // 生成 then 语句块
        koopa_ofs << then_label << ":" << endl;
        then_stmt->print();
        koopa_ofs << "\tjump " << end_label << endl;
    }
    // 生成 end 标签
    koopa_ofs << end_label << ":" << endl;
    // 恢复是否返回的记录，避免 if 生成的语句块中有单条 return 语句修改当前块 is_returned = true
    local_symbol_table->is_returned = false;
    return Result();
}
```

可以看到，我们会大量的生成 `label`，并添加对应的 `jump` 指令，虽然这带来了很多冗余指令，但恰好能完全满足 `%label` 标号块最末必须是 `br`、`jump` 或 `ret` 的要求。而且在编译 Riscv 之前，生成内存形式的 Koopa IR 时，这些生成的死代码都会被库函数自动消除掉，不需要我们关心。

### 处理作用域

考虑一个 `if(0) return 2; return 3;` 的测试点。

如果不给 `MatchedStmt` 新开符号表，那么会导致 `return 2` 影响掉当前符号表的 `is_returned`，从而导致 `return 3` 被跳过。

所以，我们还需要在 `if` 语句处理后，设置当前符号表是否返回为 `false`。

注意，`MatchedStmt` 是不会在不推导出 `Block` 下就推导出一条 `Decl` 的，所以你不必再在这里专门创建一个新的符号表。

当然，你也可以对他进行一步优化，如果这里是 `if ... else ...`，且两个分支都 `return` 了，那么就直接设置当前符号表的 `is_returned = true`。

不过，有了对于 `ret` 的标号后处理，我们其实完全不用 care 这件事。这个只影响 `BlockAST` 是否多打印一些语句罢了。

我目前选的是第一种方法。

### 跨 Block 链符号重名

现在对于符号的命名基于 `symbol_table` 的嵌套深度 `depth`，这会导致跨 Block 链的符号重名时，发生多个同名 `alloc` 指令，这是不合法的。

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

这里发现始终过不去 `11_logical1` 测试点，先 `AE` 后 `WA`。

首先是 `AE`，发现是我在处理 12 位立即数偏置的时候，错误地使用了 `reg(sp)` 的形式。

实际上偏移量不是指做成 `t1(sp)`，而是先做 `t1 = bias; t1 = sp + t1`，然后再 `lw t0, (t1)`。对 `sw` 指令同理。

接着是 `WA`，发现是我在处理 12 位立即数的时候，寄存器分配出现了问题，我手动调整了 `context.stack_used` 的值为临界值 `2040` 后，发现是我原先对于 `load` 处的寄存器分配有问题，我使用了 `cur_reg` 而不是 `new_reg`，这会导致如果 `load` 指令目标偏置超过 12 位立即数限制，那么在 `riscv._lw(reg, "sp", context.stack_map[load.src]);` 中，会隐式地发现偏置大于 `2048` 并再次分配 `t0` 来存储偏置，从而造成一句 `lw t0, (t0)` 的指令。修改为 `new_reg` 后，即可 AC。
