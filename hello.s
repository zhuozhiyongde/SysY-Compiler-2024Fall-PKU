
	.text
	.globl f
f:
	addi sp, sp, 0
f_entry:
	addi sp, sp, 0
	ret

	.text
	.globl main
main:
	addi sp, sp, -16
	sw ra, 12(sp)
main_entry:
	call f
	li a0, 1
	lw ra, 12(sp)
	addi sp, sp, 16
	ret
