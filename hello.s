
	.data
	.globl global_0
global_0:
	.word 2

	.text
	.globl main
main:
	addi sp, sp, -16
main_entry:
	li t0, 3
	la t1, global_0
	sw t0, 0(t1)
	la t0, global_0
	lw t0, 0(t0)
	sw t0, 0(sp)
	lw a0, 0(sp)
	addi sp, sp, 16
	ret
