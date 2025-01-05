# make; build/compiler -koopa hello.c -o hello.asm
# autotest -koopa -s lv8 /root/compiler
# make; build/compiler -riscv hello.c -o hello.s
# autotest -riscv -s lv8 /root/compiler


# 测试 Koopa
# koopac hello.asm | llc --filetype=obj -o hello.o
# clang hello.o -L$CDE_LIBRARY_PATH/native -lsysy -o hello
# ./hello
# rm hello.o hello

# 测试 Riscv
clang hello.s -c -o hello.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
ld.lld hello.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o hello
qemu-riscv32-static hello