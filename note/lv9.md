# Lv9

## Koopa

### 数组初始化

数组初始化的思路如下：

1. 使用 `vector<int> indices` 收集所有的数组维度
2. 计算总共的元素个数 `total`
3. 真的去堆上分配一个 `int arr[total]` 的数组，然后往里面存数据。
4. 使用递归去做初始化，递归函数签名为 `void init(const vector<int>& indices, int*& arr, int& cur, int align)`，其中：
   - `indices` 是存放数组维度的 `vector<int>`
   - `arr` 是真实分配数组指针
   - `cur` 是当前分配到的数组下标
   - `align` 是当前对齐步长

递归函数的主要逻辑如下：

1. 计算每个维度的步长（使用连乘法遍历），并存储在 `vector<int> steps` 中。
2. 如果 `init_values` 为空（即单个 `{}`），直接填充一个 `0`，避免没走遍历直接判断对齐，然后直接跳过了，填 `0` 可以保证后续对齐的时候能走满一个步长。
3. 遍历 `init_values`：
   - 如果是整数，直接赋值到 `arr` 中，并递增 `cur`。
   - 如果是初始化列表，找到合适的步长。若遍历得到的步长大于等于当前对齐尺度 `align` 则跳过。因为当前是在遍历一个列表 `init_values`，其中各个元素的对齐步长肯定不会大于等于此时的对齐步长 `align`。
   - 进行递归初始化。
4. 最后，对齐到 `align`，在 `cur` 不满足对齐要求时填充 `0`。

其他的注意事项：

首先，局部数组不能用 zeroinit 初始化，这个声明只能在全局数组中使用。

其次，要考虑形如 `{}` 的初始化，这个东西只要出现，就至少会初始化掉一个步长。

如果你发现在 `22_arr_init1` 测试点 WA，那么就很有可能是此原因导致的，你可以本地测试如下测试点：

```c
const int buf[3][3][1] = { 1,{},2 };
```

这个测试点的输出应当是

```
alloc [[[i32, 1], 3], 3], {{{1}, {0}, {2}}, {{0}, {0}, {0}}, {{0}, {0}, {0}}}
```

如果你仅仅在处理第二个 `{}` 的时候检查对齐，而不考虑其 `init_values` 为空，一上来就对齐导致完全没有补 0 进而被直接跳过，那么很容易得到：

```
alloc [[[i32, 1], 3], 3], {{{1}, {2}, {0}}, {{0}, {0}, {0}}, {{0}, {0}, {0}}}
```

另外一个测试点是：

```c
const int buf[2][3] = {{}, 1};
```

这个测试点输出应当是：

```
global @buf_0 = alloc [[i32, 3], 2], {{0, 0, 0}, {1, 0, 0}}
```

### 数组存值、访问

我新增加了两个符号类型：

- `ARR`：表示数组
- `PTR`：表示指针

他们对应的 `value` 为其定值时知道的维度个数。

- 对于 `ARR` 来说，`value` 为数组的维度个数
- 对于 `PTR` 来说，`value` 为指针指向的维度个数，若声明为 `int[]` 则记为 1，若声明为 `int[][3]` 则记为 2，以此类推。注意这里要判断对于 AST 那个数组表达式的 vector 的长度 + 1，对应那个没有显式表达式的 `[]` 维度。

为什么要这么做？因为一个数组表达式的 LVal 出现的位置是不确定的，其既可以作为值，也可以作为指针参数去调用函数，我们必须判断输出它时究竟是哪种情况，进而输出不同的 Koopa IR。

而判断的方法，就是看我们调用他们所使用的维度个数，相对于我们初始化他们时的维度个数的关系。

- 若调用时使用的维度个数等于初始化时知道的维度个数，则其为值，我们最后补上的应当是一句 `load` 指令
- 若调用时使用的维度个数小于初始化时知道的维度个数，则其为指针，我们最后补上的应当是一句 `getelemptr` 指令

而 `ARR` 和 `PTR` 在首条加载指令的时候亦有差异，所以也要分开处理一下：

- 对于 `ARR`，我们总是先使用 `getelemptr @ident, 0` 来获取首地址，
- 对于 `PTR`，我们总是先使用 `getptr @ident, 0` 来获取首地址

然后，他们都是继续使用 `getelemptr prev_reg, 0` 来获取后续的地址

除了修改 LVal 的 AST，我们还需要修改 Assign 语句的 AST，因为他们都是既可以是左值也可以是右值。

## Riscv

### 分配指令

与以往在 store 时才检查是否分配过符号表不同，数组的引入要求我们在 alloc 语句时就进行符号表的分配。

为什么？因为考虑一个局部数组的初始化

```c
int main() {
  int a[2][3] = { 1, 2, 3 };
  return 4;
}

```

其对应的 Koopa IR 是：

```asm
decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

fun @main(): i32 {
%main_entry:
	@a_2 = alloc [[i32, 3], 2]
	%0 = getelemptr @a_2, 0
	%1 = getelemptr %0, 0
	store 1, %1
	%2 = getelemptr %0, 1
	store 2, %2
	%3 = getelemptr %0, 2
	store 3, %3
	%4 = getelemptr @a_2, 1
	%5 = getelemptr %4, 0
	store 0, %5
	%6 = getelemptr %4, 1
	store 0, %6
	%7 = getelemptr %4, 2
	store 0, %7
	ret 4
%jump_0:
	ret 0
}
```

看到了吗，在 alloc 和 store 之间插入了很多的 `getelemptr` 指令来取址，这会存储中间结果，如果我们等到了 store 时在进行存储，那么就会导致数组分配不连续，后续无法计算偏移。

同时，也是为了方便我们确定栈帧，我们还是必须要提前计算确定 `alloc` 指令分配的空间，对于原来这个值总是 4，我们无所谓摆在那里，但是现在一个数组可能不再是 4，而是 40、400。

所以，有如下函数：

```cpp
int get_alloc_size(const koopa_raw_type_t ty) {
    switch (ty->tag) {
    case KOOPA_RTT_UNIT:
    case KOOPA_RTT_FUNCTION:
        return 0;
    case KOOPA_RTT_INT32:
    case KOOPA_RTT_POINTER:
        return 4;
    case KOOPA_RTT_ARRAY:
        return ty->data.array.len * get_alloc_size(ty->data.array.base);
    default:
        printf("Invalid type: %s\n", koopaRawTypeTagToString(ty->tag).c_str());
        assert(false);
    }
}

void visit(const koopa_raw_function_t& func) {
  // ...
  for (size_t i = 0; i < func->bbs.len; ++i) {
    for (size_t j = 0; j < bb->insts.len; ++j) {
        // 如果是 alloc 指令，检查是否为数组
        if (inst->kind.tag == KOOPA_RVT_ALLOC) {
            int size = get_alloc_size(inst->ty->data.pointer.base);
            alloc_size += size;
        }
      // ...
    }
  }
}

void alloc(const koopa_raw_value_t& value) {
    printf("alloc name: %s\n", value->name);
    printf("alloc rtt: %s\n", koopaRawTypeTagToString(value->ty->tag).c_str());
    int size = get_alloc_size(value->ty->data.pointer.base);
    printf("alloc size: %d\n", size);
    context.stack_map[value] = context.stack_used;
    context.stack_used += size;
}

void visit(const koopa_raw_value_t& value) {
  // ...
  const auto& kind = value->kind;
  switch (kind.tag) {
    // ...
    case KOOPA_RVT_ALLOC:
        // 访问 alloc 指令
        alloc(value);
        break;
  }
}
```

可以看到，`get_alloc_size` 函数会递归的计算出数组的大小。而且现在使用 `alloc` 函数来处理 `alloc` 指令，可以正确记录栈偏移并更新已用栈帧。

### 数组赋值

一个测试点：

```c
int main() {
  int b[2][3] = { 1, 2, 3, 4 };
  putint(b[0][1]);
  putch(10);
  return 0;
}
```

测试输出是 4 还是 2？如果是 4，那么说明你对于 `getelemptr` 指令的翻译有问题。

这是因为，`getelemptr %0, 1` 指令的翻译并不一定是 +4，而是也需要像之前一样，使用 `get_alloc_size` 函数来计算 `%0` 的偏移步长。

如上例中，前两句赋值应当翻译为：

```
	.text
	.globl main
main:
	addi sp, sp, -80
	sw ra, 76(sp)
main_entry:
---
[KOOPA_RVT_ALLOC]
---
[KOOPA_RVT_GET_ELEM_PTR]
	addi t0, sp, 0
	sw t0, 24(sp)
---
[KOOPA_RVT_GET_ELEM_PTR]
	lw t0, 24(sp)
	sw t0, 28(sp)
---
[KOOPA_RVT_STORE]
	li t0, 1
	lw t1, 28(sp)
	sw t0, 0(t1)
---
[KOOPA_RVT_GET_ELEM_PTR]
	lw t0, 24(sp)
	li t1, 1
	li t3, 2
	sll t1, t1, t3
	add t0, t0, t1
	sw t0, 32(sp)
---
[KOOPA_RVT_STORE]
	li t0, 2
	lw t1, 32(sp)
	sw t0, 0(t1)
---
```

注：这里多打印了一些分隔符，这是一个很好的 debug 方法，即：

```cpp
void visit(const koopa_raw_value_t& value) {
    // 根据指令类型判断后续需要如何访问
    const auto& kind = value->kind;
    // RVT: Raw Value Tag, 区分指令类型
    register_manager.reset();
    // DEBUG
    riscv_ofs << "---" << endl;
    riscv_ofs << "[" << koopaRawValueTagToString(kind.tag).c_str() << "]" << endl;
    // ...
}
```

### 数组访问

对于数组的访问可能发生在如下三个位置：

- `load`：加载一个数组元素/指针到寄存器
- `store`：存储寄存器到一个数组元素/指针
- `call`：调用函数，传递数组的指针

对于 `load` 和 `store`，与 lv8 中的全局变量类似，这里也需要先取得指针的值，然后进行一次解引用，生成类似如下指令（以 store 为例）：

```riscv
lw t0, sp, 24
sw t1, 0(t0)
```

这里，24 是前面 `getelemptr` 在栈上存储的中间结果的偏移，t0 是我们加载进来的指针，0(t0) 是解引用后的地址，t1 是我们要存储的值。

对于 `load` 则是：

```riscv
lw t0, sp, 24
lw t0, 0(t0)
```

这里，24 是前面 `getelemptr` 在栈上存储的中间结果的偏移，t0 是我们加载进来的指针，0(t0) 是解引用后的地址，t0 也是我们最后加载出来的值所在的寄存器。

不过，对于 `call`，我们只需要考虑其需要传递指针作为参数的情况，传递一个完全解引用得到的数组元素的情况应当在 Koopa IR 中已经被翻译为了 load 指令，就和之前相同了。

```riscv
lw a0, sp, 24
```

## Debug

### 短路求值

发现过完本地之后远程 lv9 也能全过，但是 final 会有过不去的，执行：

```bash
autotest -riscv /root/compiler
```

发现有几个先前 lv3 的测试点过不去了，仔细检查了一下发现是短路计算的问题，原先的写法是：

```cpp
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
    // ...
}
```

这种写法有一个问题，逻辑表达式返回的必然是一个布尔型，也就是要么是 1 要么是 0，所以我们在判断 `rhs` 的时候不能直接返回，而是：

1. 检查一下其值是否为立即数，若是，则返回 `rhs.value != 0`
2. 若非立即数而是寄存器，直接返回 `rhs`

也即：

```cpp
if (logical_op == LogicalOp::LOGICAL_OR) {
    // 左侧为立即数
    if (lhs.type == Result::Type::IMM) {
      if (lhs.value != 0) {
        return IMM_(1);
      }
      else {
        Result rhs = right->print();
        if (rhs.type == Result::Type::IMM) {
          return IMM_(rhs.value != 0);
        }
        else {
          return rhs;
        }
      }
    }
    // ...
}
```

对于 `&&` 运算同理，不再赘述。

### 跳转

修好上述 bug 后，发现本地所有测试：

```bash
autotest -koopa /root/compiler
autotest -riscv /root/compiler
```

都能过，远程的从 Koopa 也能过，但是 Riscv 差一个点。

搜了一下树洞，发现是存在长跳转的问题，即跳转范围超过了 `bnez` 和 `beqz` 的跳转范围，所以需要使用 `jump` 指令。

只需要修改一下这两条的指令的实现，在 `bnez` 和 `beqz` 旁边添加新的标号，然后将原先的短跳转转为长跳转 `jump` 指令即可：

```cpp
void Riscv::_bnez(const string& cond, const string& label) {
    auto target_1 = context_manager.get_branch_label();
    auto target_2 = context_manager.get_branch_end_label();
    riscv_ofs << "\tbnez " << cond << ", " << target_1 << endl;
    _jump(target_2);
    _label(target_1);
    _jump(label);
    _label(target_2);
}
```

这里 `branch_count` 是一个全局变量，可以避免标签重复。

这对于 `beqz` 同理。

修改完成后，我们也就完成了除性能测试以外的所有测试点。

### 性能测试

发现是没有在 `main.cpp` 中允许 `-perf` 的模式（其实就是和 `-riscv` 一样），添加一下就行。

测试指令：

```bash
autotest -perf -s perf /root/compiler
```

最终测试结果在 600s 左右，大概排在 67% 左右的样子，没继续做图着色优化了。
