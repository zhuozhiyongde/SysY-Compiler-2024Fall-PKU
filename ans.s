  .text
	.globl f1
f1:
	addi sp, sp, -80
	sw a0, 0(sp)
---
	sw a1, 4(sp)
---
	lw t0, 0(sp)
	sw t0, 8(sp)
---
	lw t0, 8(sp)
	li t1, 0
	li t2, 4
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 12(sp)
---
	li t0, 1
	lw t1, 12(sp)
	sw t0, 0(t1)
---
	lw t0, 0(sp)
	sw t0, 16(sp)
---
	lw t0, 16(sp)
	li t1, 0
	li t2, 4
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 20(sp)
---
	lw t0, 20(sp)
	lw t0, 0(t0)
	sw t0, 24(sp)
---
	lw t0, 24(sp)
	li t1, 1
	add t0, t0, t1
	sw t0, 28(sp)
---
	lw t0, 4(sp)
	sw t0, 32(sp)
---
	lw t0, 32(sp)
	li t1, 0
	li t2, 8
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 36(sp)
---
	lw t0, 36(sp)
	li t1, 1
	li t2, 4
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 40(sp)
---
	lw t0, 28(sp)
	lw t1, 40(sp)
	sw t0, 0(t1)
---
	lw t0, 4(sp)
	sw t0, 44(sp)
---
	lw t0, 44(sp)
	li t1, 0
	li t2, 8
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 48(sp)
---
	lw t0, 48(sp)
	li t1, 1
	li t2, 4
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 52(sp)
---
	lw t0, 52(sp)
	lw t0, 0(t0)
	sw t0, 56(sp)
---
	lw t0, 56(sp)
	li t1, 1
	add t0, t0, t1
	sw t0, 60(sp)
---
	lw t0, 4(sp)
	sw t0, 64(sp)
---
	lw t0, 64(sp)
	li t1, 1
	li t2, 8
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 68(sp)
---
	lw t0, 68(sp)
	li t1, 0
	li t2, 4
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 72(sp)
---
	lw t0, 60(sp)
	lw t1, 72(sp)
	sw t0, 0(t1)
---
	li a0, 0
	addi sp, sp, 80
  ret


  .text
	.globl main
main:
	addi sp, sp, -80
	sw ra, 76(sp)
---
	addi t0, sp, 0
	li t1, 0
	li t2, 4
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 36(sp)
---
	addi t0, sp, 12
	li t1, 0
	li t2, 8
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 40(sp)
---
	lw a0, 36(sp)
	lw a1, 40(sp)
	call f1
	sw a0, 44(sp)
---
	addi t0, sp, 0
	li t1, 0
	li t2, 4
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 48(sp)
---
	lw t0, 48(sp)
	lw t0, 0(t0)
	sw t0, 52(sp)
---
	lw a0, 52(sp)
	call putint
	li a0, 10
	call putch
	addi t0, sp, 12
	li t1, 0
	li t2, 8
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 56(sp)
---
	lw t0, 56(sp)
	li t1, 1
	li t2, 4
	mul t1, t1, t2
	add t0, t0, t1
	sw t0, 60(sp)
---
	lw t0, 60(sp)
	lw t0, 0(t0)
	sw t0, 64(sp)
---
	lw a0, 64(sp)
	call putint
	li a0, 10
	call putch
	li a0, 0
	lw ra, 76(sp)
	addi sp, sp, 80
  ret


