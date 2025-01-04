# make; build/compiler -koopa hello.c -o hello.asm
# autotest -koopa -s lv6 /root/compiler
# make; build/compiler -riscv hello.c -o hello.s
# autotest -riscv -s lv6 /root/compiler


# 测试 Koopa
koopac hello.asm | llc --filetype=obj -o hello.o
clang hello.o -L$CDE_LIBRARY_PATH/native -lsysy -o hello
./hello
rm hello.o hello

