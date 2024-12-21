	.text
	.globl main
main:
	addi sp, sp, -64
entry:
	li t0, 8192
	sw t0, 0(sp)
	lw t0, 0(sp)
	sw t0, 4(sp)
	lw t0, 4(sp)
	li t1, 8192
	xor t0, t0, t1
	seqz t0, t0
	sw t0, 8(sp)
	lw t0, 8(sp)
	bnez t0, then_0
	beqz t0, end_0
then_0:
	lw t0, 0(sp)
	sw t0, 12(sp)
	lw t0, 12(sp)
	li t1, 2
	sgt t0, t0, t1
	sw t0, 16(sp)
	lw t0, 16(sp)
	bnez t0, then_1
	beqz t0, end_1
end_0:
	li a0, -1
	addi sp, sp, 64
	ret
then_1:
	lw t0, 0(sp)
	sw t0, 20(sp)
	lw t0, 20(sp)
	li t1, 3
	slt t0, t0, t1
	sw t0, 24(sp)
	lw t0, 24(sp)
	bnez t0, then_2
	beqz t0, else_2
end_1:
	j end_0
then_2:
	li t0, 1024
	sw t0, 0(sp)
	li a0, 8192
	addi sp, sp, 64
	ret
else_2:
	lw t0, 0(sp)
	sw t0, 28(sp)
	lw t0, 28(sp)
	li t1, 4
	sgt t0, t0, t1
	sw t0, 32(sp)
	lw t0, 32(sp)
	bnez t0, then_3
	beqz t0, end_3
then_3:
	lw t0, 0(sp)
	sw t0, 36(sp)
	lw t0, 36(sp)
	li t1, 5
	slt t0, t0, t1
	sw t0, 40(sp)
	lw t0, 40(sp)
	bnez t0, then_4
	beqz t0, else_4
end_3:
	j end_2
then_4:
	lw t0, 0(sp)
	sw t0, 44(sp)
	lw t0, 44(sp)
	li t1, 1
	add t0, t0, t1
	sw t0, 48(sp)
	lw a0, 48(sp)
	addi sp, sp, 64
	ret
else_4:
	lw t0, 0(sp)
	sw t0, 52(sp)
	lw t0, 52(sp)
	li t1, 2
	add t0, t0, t1
	sw t0, 56(sp)
	lw a0, 56(sp)
	addi sp, sp, 64
	ret
end_2:
	j end_1
