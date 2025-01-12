
	.text
	.globl main
main:
	addi sp, sp, -80
	sw ra, 76(sp)
main_entry:
	addi t0, sp, 0
	sw t0, 24(sp)
	lw t0, 24(sp)
	sw t0, 28(sp)
	li t0, 1
	lw t1, 28(sp)
	sw t0, 0(t1)
	lw t0, 24(sp)
	li t1, 1
	li t3, 4
	mul t1, t1, t3
	add t0, t0, t1
	sw t0, 32(sp)
	li t0, 2
	lw t1, 32(sp)
	sw t0, 0(t1)
	lw t0, 24(sp)
	li t1, 2
	li t3, 4
	mul t1, t1, t3
	add t0, t0, t1
	sw t0, 36(sp)
	li t0, 3
	lw t1, 36(sp)
	sw t0, 0(t1)
	addi t0, sp, 0
	li t1, 1
	li t3, 4
	mul t1, t1, t3
	add t0, t0, t1
	sw t0, 40(sp) // [1][0]
	lw t0, 40(sp)
	sw t0, 44(sp) // [1][0]
	li t0, 4
	lw t1, 44(sp)
	sw t0, 0(t1) // [1][0] = 4
	lw t0, 40(sp)
	li t1, 1
	li t3, 4
	mul t1, t1, t3
	add t0, t0, t1
	sw t0, 48(sp)
	lw t0, 48(sp)
	sw x0, 0(t0)
	lw t0, 40(sp)
	li t1, 2
	li t3, 4
	mul t1, t1, t3
	add t0, t0, t1
	sw t0, 52(sp)
	lw t0, 52(sp)
	sw x0, 0(t0) // [1][2] = 0

	addi t0, sp, 0
	sw t0, 56(sp)
	lw t0, 56(sp) // [0][0]
	li t1, 1
	li t3, 4
	mul t1, t1, t3
	add t0, t0, t1 
	sw t0, 60(sp)
	lw t0, 60(sp)
	lw t0, 0(t0)
	sw t0, 64(sp)
	lw t0, 64(sp)
	mv a0, t0
	call putint


	li a0, 10
	call putch
	li a0, 0
	lw ra, 76(sp)
	addi sp, sp, 80
	ret
