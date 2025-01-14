# 将 hello.c 编译为 Koopa IR
# make && build/compiler -koopa hello.c -o hello.asm

# 本地运行对于 Koopa 的测试
# autotest -koopa -s lv9 /root/compiler

# 将 hello.c 编译为 Riscv 汇编
# make && build/compiler -riscv hello.c -o hello.s

# 本地运行对于 Riscv 的测试
# autotest -riscv -s lv9 /root/compiler

# 测试 Koopa
# make && build/compiler -koopa hello.c -o hello.asm
# echo "==========="
# koopac hello.asm | llc --filetype=obj -o hello.o
# clang hello.o -L$CDE_LIBRARY_PATH/native -lsysy -o hello
# echo "==========="
# ./hello
# rm hello.o hello

# 测试 Riscv
# make && build/compiler -riscv hello.c -o hello.s
# echo "==========="
# clang hello.s -c -o hello.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
# ld.lld hello.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o hello
# echo "==========="
# qemu-riscv32-static hello
# rm hello.o hello