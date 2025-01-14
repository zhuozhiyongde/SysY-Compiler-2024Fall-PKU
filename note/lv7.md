# Lv7

## Koopa

### 短路求值

在写 lv7 之前，先处理掉了一些写 lv6 的时候的问题。

首先是重新组织了一下文件，将 visit.cpp / visit.hpp 全部改成了 asm.cpp / asm.hpp。

增加了 `tmp_reg()` 函数，用于返回一个临时寄存器，然后恢复寄存器计数。

然后，移除了 `add_sp` 指令，将之改为更加泛化的 `addi` 指令，并修改了相关代码。

我还将前端的 `ContextManager` 全部重命名为了 `EnvironmentManager`，避免和后端冲突。

（其实我尝试了使用命名空间来进行区分，但是寄了...后来者或许可以在初期就尝试使用命名空间来进行区分）

最后，就是实现了一下短路求值代码的修改。

短路求值只需要修改 Koopa 即可，不需要修改后端。

```cpp
Result LExpWithOpAST::print() const {
    // 先打印计算左表达式结果的语句，并获取左表达式结果
    Result lhs = left->print();
    // 根据逻辑运算符的类型，打印逻辑运算语句
    // 逻辑或运算符
    if (logical_op == LogicalOp::LOGICAL_OR) {
        // 左侧为立即数
        if (lhs.type == Result::Type::IMM) {
            // 如果左侧为立即数，且值不为 0，则直接返回 1，进行短路求值
            if (lhs.value != 0) {
                return IMM_(1);
            }
            // 如果左侧为立即数，且值为 0，则计算右表达式结果
            else {
                Result rhs = right->print();
                // 如果右表达式结果为立即数，则直接返回右表达式结果
                if (rhs.type == Result::Type::IMM) {
                    return IMM_(rhs.value != 0);
                }
                else {
                    // 生成一条 ne 0 指令，相当于 rhs != 0，得到布尔值
                    koopa_ofs << "\t" << NEW_REG_ << " = ne " << rhs << ", 0" << endl;
                    return CUR_REG_;
                }
            }
        }
        // 左侧不为立即数
        auto true_label = environment_manager.get_short_true_label();
        auto false_label = environment_manager.get_short_false_label();
        auto end_label = environment_manager.get_short_end_label();
        auto result = environment_manager.get_short_result_reg();
        environment_manager.add_short_circuit_count();

        // 生成 alloc 指令
        koopa_ofs << "\t" << result << " = alloc i32" << endl;
        environment_manager.is_symbol_allocated[result] = true;

        // 生成 br 指令
        koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;

        // 生成 true 分支
        koopa_ofs << true_label << ":" << endl;
        koopa_ofs << "\t" << "store 1, " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        // 生成 false 分支
        koopa_ofs << false_label << ":" << endl;
        Result rhs = right->print();
        Result temp = NEW_REG_;
        // 生成一条 ne 0 指令，相当于 rhs != 0，得到布尔值
        koopa_ofs << "\t" << temp << " = ne " << rhs << ", 0" << endl;
        koopa_ofs << "\t" << "store " << temp << ", " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        // 生成 end 标签
        koopa_ofs << end_label << ":" << endl;
        Result result_reg = NEW_REG_;
        koopa_ofs << "\t" << result_reg << " = load " << result << endl;

        return result_reg;
    }
    // 逻辑与运算符
    else if (logical_op == LogicalOp::LOGICAL_AND) {
        // 左侧为立即数
        if (lhs.type == Result::Type::IMM) {
            if (lhs.value == 0) {
                return IMM_(0);
            }
            else {
                Result rhs = right->print();
                if (rhs.type == Result::Type::IMM) {
                    return IMM_(rhs.value != 0);
                }
                else {
                    // 生成一条 ne 0 指令，相当于 rhs != 0，得到布尔值
                    koopa_ofs << "\t" << NEW_REG_ << " = ne " << rhs << ", 0" << endl;
                    return CUR_REG_;
                }
            }
        }
        // 左侧不为立即数
        auto true_label = environment_manager.get_short_true_label();
        auto false_label = environment_manager.get_short_false_label();
        auto end_label = environment_manager.get_short_end_label();
        auto result = environment_manager.get_short_result_reg();
        environment_manager.add_short_circuit_count();

        // 生成 alloc 指令
        koopa_ofs << "\t" << result << " = alloc i32" << endl;
        environment_manager.is_symbol_allocated[result] = true;

        // 生成 br 指令
        koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;

        // 生成 false 分支
        koopa_ofs << false_label << ":" << endl;
        koopa_ofs << "\t" << "store 0, " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        // 生成 true 分支
        koopa_ofs << true_label << ":" << endl;
        Result rhs = right->print();
        Result temp = NEW_REG_;
        // 生成一条 ne 0 指令，相当于 rhs != 0，得到布尔值
        koopa_ofs << "\t" << temp << " = ne " << rhs << ", 0" << endl;
        koopa_ofs << "\t" << "store " << temp << ", " << result << endl;
        koopa_ofs << "\tjump " << end_label << endl;

        // 生成 end 标签
        koopa_ofs << end_label << ":" << endl;
        Result result_reg = NEW_REG_;
        koopa_ofs << "\t" << result_reg << " = load " << result << endl;

        return result_reg;
    }
    else {
        assert(false);
    }
}
```

可以看到，为了实现短路求值，我们必须像 `if` 语句一样，获取一些 `label` 来控制逻辑流。

同时，对于其结果，我们不能使用类似 `%num` 这样的寄存器来存储，而必须存储在 `@short_result_0` 这样的变量中（且需要先使用 `alloc` 分配），因为我们在运行的时候并不可知其结果，根据 `br` 的跳转与否，我们可能会发生类似 ICS 课程中的 `cmov` 指令，而传统的寄存器是不支持多次赋值的。

而且需要注意一下，对于逻辑表达式，其返回值一定是一个布尔类型，所以你需要考虑如下的测试点，其不能被用常量传播直接求出：

```cpp
int main() {
    int x = 2;
    putint(0||x);
		putch(10);
}
```

这个点的输出应当是 1，而不是 2。这个测试点甚至在全部的本地/在线测试中都不存在类似的，导致我直到通过了所有测试开始逐行加注释的时候才发现。


### while 语句

`while` 语句的处理需要实现三个标签：

- `%while_entry`：while 语句的入口标签
- `%while_body`：while 语句的循环体标签
- `%while_end`：while 语句的结束标签

这三个标签都会作为跳转语句的目标，其中 `%while_entry` 还会作为 `continue` 语句的跳转目标，`%while_end` 还会作为 `break` 语句的跳转目标。

我们还需要为 `while` 循环维护两个计数器：

- `while_count`：`while` 语句计数，用于生成 `label`
- `while_current`：当前正在处理的 `while` 语句标号，用于控制 `break` / `continue` 的目的地

值得注意的是，如果你的 `if` 语句实现的比较成熟，那么 `while` 语句的实现将会非常简单，几乎没有什么坑点，除了：

1. 由于你不知道 `while` 语句是不是某一标签后第一个语句，所以你需要在 `while` 语句的开始处生成一个 `jump` 指令，跳转到 `while` 语句的入口标签。
2. 在 `break` / `continue` 语句中，你应当有形如 `ret` 语句的处理，使得最后一条语句变为标签而不是跳转指令。在我的实现中，我合并了 `ret` / `break` / `continue` 的语句计数，统一改为 `jump_count`。

## Riscv

啥都不用改，一遍 AC，从未如此美妙！