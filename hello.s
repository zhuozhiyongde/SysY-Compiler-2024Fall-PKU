	.text
	.globl main
main:
	addi sp, sp, -16
	li t0, 1
	sw t0, 0(sp)
	li a0, 0
	addi sp, sp, 16
	ret
