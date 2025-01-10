# Lv8

## Koopa

必须修改掉 FuncType 的定义，不然会在 Decl 和 FuncDef 中间出现规约/移进冲突。

不用考虑同名函数依照参数列表的重载，但要考虑函数和变量同名的情况。

参数列表可以是 0、1 或者更多个。

load 和 store 指令的前缀需要适配，要注意是 `@` 开头还是 `%` 开头。

写完后，发现在 16_summary1 测试点上 `AE` 了，仔细检查尝试，发现了问题，即在不同函数体内可能声明同样的变量：

```c
int f() {
  int a = 1;
}

int g() {
  int a = 2;
}
```

这意味着你需要在每次进入函数体时清空 is_symbol_allocated，在两个函数体内各自生成一次 `alloc` 指令。

修改完后，就一遍 AC 了。

## Riscv

首先要实现 call 指令。

十分抽象的点：由于目前为止，我们的所有中间变量都是保存在栈上的，所以我们多数情况下，无需考虑被调用者保存寄存器的问题，除了 `ra` 寄存器。

在生成 Koopa 的时候，其实是允许基本块标号（label）在不同函数内重复的，可是当生成 Riscv 的时候，全局的标号应当彼此不同。由于我们生成 Koopa 的时候，大部分的跳转标号都是基于全局变量（EnvironmentManager）来做的，所以彼此之间不会冲突，但是对于函数体进入时的 `%entry` 标号，则会造成冲突。

所以这里有两种做法：

1. 回到 KoopaIR 生成时，将生成 `%entry` 标号改为 `%<ident>_entry`，这样就不会冲突了。
2. 在 Riscv 生成时（即处理基本块 `void visit(const koopa_raw_basic_block_t& bb)` 时），特判一下当前遇到的是不是 `%entry`，如果是，则改为 `%<ident>_entry`。

我选择的是第一种做法，因为我觉得这更统一一些。

对于 void 函数，其最后处理 `void visit(const koopa_raw_return_t& ret)` 时，可能会存在 `ret.value` 为空指针的情况，需要特判，补一个 `ret` 指令即可。

还要处理 `ra` 寄存器，这个寄存器存储了返回地址，我们需要保证其在进入某函数和退出某函数时一致。而这个寄存器可能会被 `call` 指令修改，所以我们可以选择遍历所有指令看看有没有 `call` 指令，若有则处理备份，若无则不处理。

或者也可以更简单粗暴，无论有没有都进行处理。不过本来就需要遍历 `call` 指令来确定帧栈大小，所以还是选择第一种做法。

要注意，在 ret 的时候，要先恢复 ra，再恢复栈指针。因为一旦恢复栈指针，那么栈指针就变到了调用者栈帧的栈底。

首先处理 call 函数处理参数的部分，由于我们每次 call 的时候都要压栈的参数在栈底，也即相对 sp 指针偏移 0 ~ 4*(n-8) 的部分，所以我们在初始化栈帧的时候就要考虑这点，提前获取到所有 call 指令中最大的压栈参数个数，然后初始化栈帧并设置 context.stack_used 为这个值，这样才能保证 call 指令的参数压栈不会出错（不同 call 指令之间可以覆写这个区域，无所谓）。

我们还需要处理函数开头获取参数的部分，这需要我们修改 get_operand_reg 函数，使其能够处理 `KOOPA_RVT_FUNC_ARG_REF` 类型。

接着，要实现全局变量。

首先添加处理函数：

```c
void visit(const koopa_raw_value_t& value) {
    // 根据指令类型判断后续需要如何访问
    const auto& kind = value->kind;
    // RVT: Raw Value Tag, 区分指令类型
    register_manager.reset();
    // printf("visit: %s\n", koopaRawValueTagToString(kind.tag).c_str());
    switch (kind.tag) {
    // ...
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 访问 global_alloc 指令
        visit(kind.data.global_alloc, value);
        break;
    // ...
    }
}
```

然后实现它。

观察文档给出的两个生成示例：

```
  .data
  .globl var
var:
  .zero 4
```

和

```
	.data
	.globl var_0
var_0:
	.word 233
```

得知，我们需要处理两类全局变量：

1. 未初始化全局变量
2. 已初始化全局变量

由于我们的 Riscv 生成时，文本形式的 Koopa IR 已经被转入内存形式，也就不再具有了字符形式的变量名，所以需要我们手动维护全局变量名。

这通过在 `ContextManager` 中维护一个 `global_map` 来实现：

```c
/**
 * @brief ContextManager 类，用于管理 Context
 * @note - context_map：Context 映射，用于管理 Context 的使用情况
 */
class ContextManager {
private:
    int global_count = 0;
public:
    unordered_map<string, Context> context_map;
    unordered_map<koopa_raw_value_t, string> global_map;
    void create_context(const string& name, int stack_size);
    void create_global(const koopa_raw_value_t& value);
    Context& get_context(const string& name);
    string get_global(const koopa_raw_value_t& value);
};
```

这样，我们就能维护出一个全局变量分配指令（`global_alloc`，语句类型为 `KOOPA_RVT_GLOBAL_ALLOC`）到变量名的映射。

```c
void visit(const koopa_raw_global_alloc_t& global_alloc, const koopa_raw_value_t& value) {
    // 访问 global_alloc 指令
    riscv_ofs << endl;
    context_manager.create_global(value);
    auto global_name = context_manager.get_global(value);
    riscv._data();
    riscv._globl(global_name);
    riscv._label(global_name);
    auto init = global_alloc.init;
    switch (init->kind.tag) {
    case KOOPA_RVT_INTEGER:
        riscv._word(init->kind.data.integer.value);
        break;
    case KOOPA_RVT_ZERO_INIT:
        riscv._zero(4);
        break;
    default:
        assert(false);
    }
}
```

然后，由于新增了全局变量，所以我们的 `load` 和 `store` 指令也需要适配。

在以前的实现中，只要我们使用了 `load`，那么前面必定有 `store` 指令，所以我们可以保证 `load.src` 一定是在 `context_map` 中有所维护的，但是现在变量的位置可以不在栈上而是在堆上（即全局变量），所以需要在 `load` 内特判一下，如果 `load.src` 是全局变量（`load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC`），则需要生成如下指令：

```
  la t0, global_0 // 获取全局变量地址
  lw t0, 0(t0) // 获取全局变量的值
```

```cpp
void visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value) {
    auto reg = register_manager.new_reg();
    auto bias = context.stack_used;
    printf("load: %s\n", koopaRawValueTagToString(load.src->kind.tag).c_str());
    // 如果是全局变量，需要先获取地址，再获取值
    if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        riscv._la(reg, context_manager.get_global(load.src));
        riscv._lw(reg, reg, 0);
    }
    // 如果是栈上变量，直接获取值
    else {
        riscv._lw(reg, "sp", context.stack_map[load.src]);
    }
    context.push(value, bias);
    riscv._sw(reg, "sp", bias);
}
```

对于 store 指令，也需要特判，生成类似如下指令：

```
	la t1, global_0 // 获取全局变量地址
	sw t0, 0(t1) // 存储值到全局变量
```

```cpp
void visit(const koopa_raw_store_t& store) {
    printf("store value: %s\n", koopaRawValueTagToString(store.value->kind.tag).c_str());
    printf("store dest: %s\n", koopaRawValueTagToString(store.dest->kind.tag).c_str());
    register_manager.get_operand_reg(store.value);
    // 如果是全局变量，需要先获取地址，再存储
    if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        auto reg = register_manager.new_reg();
        riscv._la(reg, context_manager.get_global(store.dest));
        riscv._sw(register_manager.reg_map[store.value], reg, 0);
        return;
    }
    // 如果是栈上变量，且 dest 不在 stack_map 中（我们没处理过 alloc），则需要分配新的空间
    if (context.stack_map.find(store.dest) == context.stack_map.end()) {
        context.push(store.dest, context.stack_used);
    }
    assert(register_manager.reg_map[store.value] != "");
    riscv._sw(register_manager.reg_map[store.value], "sp", context.stack_map[store.dest]);
}
```