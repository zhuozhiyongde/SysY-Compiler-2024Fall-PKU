    .text
    .globl main
main:
    sw ra, -4(sp)
    sw fp, -8(sp)
    mv fp, sp
    addi sp, sp, -64
entry_main:
    li t0, 0
    bnez t0, then1
    beqz t0, if_end1
then1:
    li a0, 1
    j func_return_main
if_end1:
    li a0, 1
    j func_return_main
func_return_main:
    addi sp, sp, 64
    mv sp, fp
    lw fp, -8(sp)
    lw ra, -4(sp)
    ret

