
	.text
	.globl f2d
f2d:
	addi sp, sp, -32
f2d_entry:
	sw a0, 0(sp)
	lw t0, 0(sp)
	sw t0, 8(sp)
	lw t0, 8(sp)
	li t1, 3
	li t3, 2
	sll t1, t1, t3
	add t0, t0, t1
	sw t0, 12(sp)
	lw t0, 12(sp)
	lw t0, 0(t0)
	sw t0, 16(sp)
	lw t0, 16(sp)
	sw t0, 4(sp)
	addi sp, sp, 32
	ret

	.text
	.globl main
main:
	addi sp, sp, -16
	sw ra, 12(sp)
main_entry:
	li t0, 2
	sw t0, 0(sp)
	lw t0, 0(sp)
	sw t0, 4(sp)
	lw a0, 4(sp)
	call putint
	li a0, 10
	call putch
	li a0, 0
	lw ra, 12(sp)
	addi sp, sp, 16
	ret
