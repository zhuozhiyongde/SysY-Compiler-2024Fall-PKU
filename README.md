# SysY-Compiler-2024Fall-PKU

此为 `work` 分支，包含完整的开发记录。

## 文件夹结构

```
.
├── note
├── src
├── docker-compose.yml
├── get
├── hello.asm
├── hello.c
├── hello.s
├── Makefile
├── README.md
└── test.sh
```

其中：

- `note`：详细的各阶段开发笔记，在整体完成后有回顾检查，可能和 `main` 分支的对应阶段源码有出入。
- `src`：源代码
- `get`：一个获取本地测试点源码的工具，语法为 `./get [lv] [id]`，其中 `id` 可能要前补 0，或者你也可以直接在容器内运行 `cp -r /opt/bin/testcases/ .`
- `hello.[c/asm/s]`：测试编译使用的的源代码 / Koopa IR / RISC-V 结果，在容器外创建可以编辑
- `test.sh`：本地测试所需指令记录，可根据需求自行注释 / 取消注释其中部分行。

源代码目录进一步展开：

```
src
├── include
│   ├── asm.hpp
│   ├── ast.hpp
│   ├── frontend_utils.hpp
│   ├── backend_utils.hpp
│   ├── other_utils.hpp
│   └── koopa.h
├── asm.cpp
├── ast.cpp
├── frontend_utils.cpp
├── backend_utils.cpp
├── other_utils.cpp
├── main.cpp
├── sysy.l
└── sysy.y
```

其中：

- `main.cpp`：主程序，包含编译器的入口函数
- `sysy.l` 和 `sysy.y`：词法（Flex） / 语法（Bison）分析的源文件，解析源代码为抽象语法树 AST
- `ast.cpp`：前端部分，负责在语法分析的结果基础上进行语义分析，将解析得到的 AST 输出 Koopa IR
- `frontend_utils.cpp`：前端辅助部分，包括一些数据结构定义、全局变量等
- `asm.cpp`：后端部分，负责将 Koopa IR 转换为 RISC-V 汇编
- `backend_utils.cpp`：后端辅助部分，包括一些数据结构定义、全局变量等
- `other_utils.cpp`：一些其他辅助函数，主要是用于调试 `koopa.h` 库

## 本地测试

### Docker

启动容器：

```bash
docker compose up -d
```

想要进入容器 CLI，可以输入：

```bash
docker exec -it compiler bash -c "cd compiler; bash"
```

出于便利性考虑，亦可以直接将之追加到 `~/.zshrc` 或者 `~/.bashrc`：

```bash
alias qwe='docker exec -it compiler bash -c "cd compiler; bash"'
```

### 测试前端（Koopa IR 中间代码生成）

```bash
# 将 hello.c 编译为 Koopa IR
make && build/compiler -koopa hello.c -o hello.asm

# 本地运行对于 Koopa 的测试
autotest -koopa -s lv9 /root/compiler

# 测试 Koopa 输出结果
make && build/compiler -koopa hello.c -o hello.asm
echo "==========="
koopac hello.asm | llc --filetype=obj -o hello.o
clang hello.o -L$CDE_LIBRARY_PATH/native -lsysy -o hello
echo "==========="
./hello
rm hello.o hello
```

你可以适当的在源代码中添加 `putint`、`putch` 来调试，如：

```cpp
int main(){
    int a = 1;
    putint(a); // 在终端输出 a 的值
    putch(10); // 在终端初始 ASCII = 10，也即换行符
    return;
}
```

 也可以在 Koopa IR 中输出，打印某个 SSA 的结果，如原始代码为：

```cpp
int main() {
    int a = 1;
    int b = 2;
    int c = a + b;
    return c;
}
```

编译结果为：

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
	@a_2 = alloc i32
	store 1, @a_2
	@b_2 = alloc i32
	store 2, @b_2
	@c_2 = alloc i32
	%0 = load @a_2
	%1 = load @b_2
	%2 = add %0, %1
	store %2, @c_2
	%3 = load @c_2
	ret %3
%jump_0:
	ret 0
}

```

如果你想看中间结果 `%2`，只需要额外添加几行：

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
	@a_2 = alloc i32
	store 1, @a_2
	@b_2 = alloc i32
	store 2, @b_2
	@c_2 = alloc i32
	%0 = load @a_2
	%1 = load @b_2
	%2 = add %0, %1
	// ---
	// 添加下面这两行
	call @putint(%3)
	call @putch(10)
	// ---
	store %2, @c_2
	%3 = load @c_2
	ret %3
%jump_0:
	ret 0
}

```

### 测试后端（RISC-V 目标代码生成）

```bash
# 将 hello.c 编译为 Riscv 汇编
make && build/compiler -riscv hello.c -o hello.s

# 本地运行对于 Riscv 的测试
autotest -riscv -s lv9 /root/compiler

# 测试 Riscv
make && build/compiler -riscv hello.c -o hello.s
echo "==========="
clang hello.s -c -o hello.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
ld.lld hello.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o hello
echo "==========="
qemu-riscv32-static hello
rm hello.o hello
```

## 最终测评结果

- 本地测试 / 远程测试：全部 AC
- 性能测试：609ms

## 最终语法

```ebnf
CompUnit      ::= [CompUnit] (Decl | FuncDef);

Decl          ::= ConstDecl | VarDecl;
ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
BType         ::= "int";
ConstDef      ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal;
ConstInitVal  ::= ConstExp | "{" [ConstInitVal {"," ConstInitVal}] "}";
VarDecl       ::= BType VarDef {"," VarDef} ";";
VarDef        ::= IDENT {"[" ConstExp "]"}
                | IDENT {"[" ConstExp "]"} "=" InitVal;
InitVal       ::= Exp | "{" [InitVal {"," InitVal}] "}";

FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;
FuncType      ::= "void" | "int";
FuncFParams   ::= FuncFParam {"," FuncFParam};
FuncFParam    ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];

Block         ::= "{" {BlockItem} "}";
BlockItem     ::= Decl | Stmt;
Stmt          ::= LVal "=" Exp ";"
                | [Exp] ";"
                | Block
                | "if" "(" Exp ")" Stmt ["else" Stmt]
                | "while" "(" Exp ")" Stmt
                | "break" ";"
                | "continue" ";"
                | "return" [Exp] ";";

Exp           ::= LOrExp;
LVal          ::= IDENT {"[" Exp "]"};
PrimaryExp    ::= "(" Exp ")" | LVal | Number;
Number        ::= INT_CONST;
UnaryExp      ::= PrimaryExp | IDENT "(" [FuncRParams] ")" | UnaryOp UnaryExp;
UnaryOp       ::= "+" | "-" | "!";
FuncRParams   ::= Exp {"," Exp};
MulExp        ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
AddExp        ::= MulExp | AddExp ("+" | "-") MulExp;
RelExp        ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
EqExp         ::= RelExp | EqExp ("==" | "!=") RelExp;
LAndExp       ::= EqExp | LAndExp "&&" EqExp;
LOrExp        ::= LAndExp | LOrExp "||" LAndExp;
ConstExp      ::= Exp;
```