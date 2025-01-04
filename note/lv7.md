# Lv7

## Koopa

### 短路求值

在写 lv7 之前，先处理掉了一些写 lv6 的时候的问题。

首先是重新组织了一下文件，将 visit.cpp / visit.hpp 全部改成了 asm.cpp / asm.hpp。

增加了 `tmp_reg()` 函数，用于返回一个临时寄存器，然后恢复寄存器计数。

然后，移除了 `add_sp` 指令，将之改为更加泛化的 `addi` 指令，并修改了相关代码。

我还将前端的 ContextManager 全部重命名为了 EnvironmentManager，避免和后端冲突。

（其实我尝试了使用命名空间来进行区分，但是寄了...后来者或许可以在初期就尝试使用命名空间来进行区分）

最后，就是实现了一下短路求值代码的修改。

短路求值只需要修改 Koopa 即可，不需要修改后端。

```cpp

Result LExpWithOpAST::print() const {
  // 先计算左表达式结果
  Result lhs = left->print();

  if (logical_op == LogicalOp::LOGICAL_OR) {
    // 左侧为立即数
    if (lhs.type == Result::Type::IMM) {
      if (lhs.value != 0) {
        return IMM_(1);
      }
      else {
        Result rhs = right->print();
        return rhs;
      }
    }
    // 左侧不为立即数
    auto true_label = environment_manager.get_short_true_label();
    auto false_label = environment_manager.get_short_false_label();
    auto end_label = environment_manager.get_short_end_label();
    auto result = environment_manager.get_short_result_reg();
    environment_manager.add_short_circuit_count();

    // 判断是否需要生成 alloc 指令
    if (!environment_manager.is_symbol_allocated[result]) {
      koopa_ofs << "\t" << result << " = alloc i32" << endl;
      environment_manager.is_symbol_allocated[result] = true;
    }

    koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;
    koopa_ofs << true_label << ":" << endl;
    koopa_ofs << "\t" << "store 1, " << result << endl;
    koopa_ofs << "\tjump " << end_label << endl;

    koopa_ofs << false_label << ":" << endl;
    Result rhs = right->print();
    Result temp_1 = NEW_REG_;
    Result temp_2 = NEW_REG_;
    koopa_ofs << "\t" << temp_1 << " = or " << rhs << ", 0" << endl;
    koopa_ofs << "\t" << temp_2 << " = ne " << temp_1 << ", 0" << endl;
    koopa_ofs << "\t" << "store " << temp_2 << ", " << result << endl;
    koopa_ofs << "\tjump " << end_label << endl;
    koopa_ofs << end_label << ":" << endl;
    Result result_reg = NEW_REG_;
    koopa_ofs << "\t" << result_reg << " = load " << result << endl;

    return result_reg;
  }
  else if (logical_op == LogicalOp::LOGICAL_AND) {
    // 左侧为立即数
    if (lhs.type == Result::Type::IMM) {
      if (lhs.value == 0) {
        return IMM_(0);
      }
      else {
        Result rhs = right->print();
        return rhs;
      }
    }
    // 左侧不为立即数
    auto true_label = environment_manager.get_short_true_label();
    auto false_label = environment_manager.get_short_false_label();
    auto end_label = environment_manager.get_short_end_label();
    auto result = environment_manager.get_short_result_reg();
    environment_manager.add_short_circuit_count();

    // 判断是否需要生成 alloc 指令
    if (!environment_manager.is_symbol_allocated[result]) {
      koopa_ofs << "\t" << result << " = alloc i32" << endl;
      environment_manager.is_symbol_allocated[result] = true;
    }

    koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;
    koopa_ofs << false_label << ":" << endl;
    koopa_ofs << "\t" << "store 0, " << result << endl;
    koopa_ofs << "\tjump " << end_label << endl;

    koopa_ofs << true_label << ":" << endl;
    Result rhs = right->print();
    Result temp_1 = NEW_REG_;
    Result temp_2 = NEW_REG_;
    Result temp_3 = NEW_REG_;
    koopa_ofs << "\t" << temp_1 << " = ne " << lhs << ", 0" << endl;
    koopa_ofs << "\t" << temp_2 << " = ne " << rhs << ", 0" << endl;
    koopa_ofs << "\t" << temp_3 << " = and " << temp_1 << ", " << temp_2 << endl;
    koopa_ofs << "\t" << "store " << temp_3 << ", " << result << endl;
    koopa_ofs << "\tjump " << end_label << endl;
    koopa_ofs << end_label << ":" << endl;
    Result result_reg = NEW_REG_;
    koopa_ofs << "\t" << result_reg << " = load " << result << endl;

    return result_reg;
  }
}
```

可以看到，为了实现短路求值，我们必须像 if 语句一样，获取一些 label 来控制逻辑流。

同时，对于其结果，我们不能使用类似 `%num` 这样的寄存器来存储，而必须存储在 `@short_result_0` 这样的变量中（且需要先使用 `alloc` 分配），因为我们在运行的时候并不可知其结果，根据 `br` 的跳转与否，我们可能会发生类似 ICS 课程中的 `cmov` 指令，而传统的寄存器是不支持多次赋值的。


### 