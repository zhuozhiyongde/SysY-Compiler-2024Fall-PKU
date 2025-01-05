
	.text
	.globl sum
sum:
	addi sp, sp, -96
sum_entry:
	sw a0, 0(sp)
	sw a1, 4(sp)
	sw a2, 8(sp)
	sw a3, 12(sp)
	sw a4, 16(sp)
	sw a5, 20(sp)
	sw a6, 24(sp)
	sw a7, 28(sp)
	lw t0, 0(sp)
	sw t0, 32(sp)
	lw t0, 4(sp)
	sw t0, 36(sp)
	lw t0, 32(sp)
	lw t1, 36(sp)
	add t0, t0, t1
	sw t0, 40(sp)
	lw t0, 8(sp)
	sw t0, 44(sp)
	lw t0, 40(sp)
	lw t1, 44(sp)
	add t0, t0, t1
	sw t0, 48(sp)
	lw t0, 12(sp)
	sw t0, 52(sp)
	lw t0, 48(sp)
	lw t1, 52(sp)
	add t0, t0, t1
	sw t0, 56(sp)
	lw t0, 16(sp)
	sw t0, 60(sp)
	lw t0, 56(sp)
	lw t1, 60(sp)
	add t0, t0, t1
	sw t0, 64(sp)
	lw t0, 20(sp)
	sw t0, 68(sp)
	lw t0, 64(sp)
	lw t1, 68(sp)
	add t0, t0, t1
	sw t0, 72(sp)
	lw t0, 24(sp)
	sw t0, 76(sp)
	lw t0, 72(sp)
	lw t1, 76(sp)
	add t0, t0, t1
	sw t0, 80(sp)
	lw t0, 28(sp)
	sw t0, 84(sp)
	lw t0, 80(sp)
	lw t1, 84(sp)
	add t0, t0, t1
	sw t0, 88(sp)
	lw a0, 88(sp)
	addi sp, sp, 96
	ret

	.text
	.globl sum2
sum2:
	addi sp, sp, -192
sum2_entry:
	sw a0, 0(sp)
	sw a1, 4(sp)
	sw a2, 8(sp)
	sw a3, 12(sp)
	sw a4, 16(sp)
	sw a5, 20(sp)
	sw a6, 24(sp)
	sw a7, 28(sp)
	lw t0, 192(sp)
	sw t0, 32(sp)
	lw t0, 196(sp)
	sw t0, 36(sp)
	lw t0, 200(sp)
	sw t0, 40(sp)
	lw t0, 204(sp)
	sw t0, 44(sp)
	lw t0, 208(sp)
	sw t0, 48(sp)
	lw t0, 212(sp)
	sw t0, 52(sp)
	lw t0, 216(sp)
	sw t0, 56(sp)
	lw t0, 220(sp)
	sw t0, 60(sp)
	lw t0, 0(sp)
	sw t0, 64(sp)
	lw t0, 4(sp)
	sw t0, 68(sp)
	lw t0, 64(sp)
	lw t1, 68(sp)
	add t0, t0, t1
	sw t0, 72(sp)
	lw t0, 8(sp)
	sw t0, 76(sp)
	lw t0, 72(sp)
	lw t1, 76(sp)
	add t0, t0, t1
	sw t0, 80(sp)
	lw t0, 12(sp)
	sw t0, 84(sp)
	lw t0, 80(sp)
	lw t1, 84(sp)
	add t0, t0, t1
	sw t0, 88(sp)
	lw t0, 16(sp)
	sw t0, 92(sp)
	lw t0, 88(sp)
	lw t1, 92(sp)
	add t0, t0, t1
	sw t0, 96(sp)
	lw t0, 20(sp)
	sw t0, 100(sp)
	lw t0, 96(sp)
	lw t1, 100(sp)
	add t0, t0, t1
	sw t0, 104(sp)
	lw t0, 24(sp)
	sw t0, 108(sp)
	lw t0, 104(sp)
	lw t1, 108(sp)
	add t0, t0, t1
	sw t0, 112(sp)
	lw t0, 28(sp)
	sw t0, 116(sp)
	lw t0, 112(sp)
	lw t1, 116(sp)
	add t0, t0, t1
	sw t0, 120(sp)
	lw t0, 32(sp)
	sw t0, 124(sp)
	lw t0, 120(sp)
	lw t1, 124(sp)
	add t0, t0, t1
	sw t0, 128(sp)
	lw t0, 36(sp)
	sw t0, 132(sp)
	lw t0, 128(sp)
	lw t1, 132(sp)
	add t0, t0, t1
	sw t0, 136(sp)
	lw t0, 40(sp)
	sw t0, 140(sp)
	lw t0, 136(sp)
	lw t1, 140(sp)
	add t0, t0, t1
	sw t0, 144(sp)
	lw t0, 44(sp)
	sw t0, 148(sp)
	lw t0, 144(sp)
	lw t1, 148(sp)
	add t0, t0, t1
	sw t0, 152(sp)
	lw t0, 48(sp)
	sw t0, 156(sp)
	lw t0, 152(sp)
	lw t1, 156(sp)
	add t0, t0, t1
	sw t0, 160(sp)
	lw t0, 52(sp)
	sw t0, 164(sp)
	lw t0, 160(sp)
	lw t1, 164(sp)
	add t0, t0, t1
	sw t0, 168(sp)
	lw t0, 56(sp)
	sw t0, 172(sp)
	lw t0, 168(sp)
	lw t1, 172(sp)
	add t0, t0, t1
	sw t0, 176(sp)
	lw t0, 60(sp)
	sw t0, 180(sp)
	lw t0, 176(sp)
	lw t1, 180(sp)
	add t0, t0, t1
	sw t0, 184(sp)
	lw a0, 184(sp)
	addi sp, sp, 192
	ret

	.text
	.globl main
main:
	addi sp, sp, -64
	sw ra, 60(sp)
main_entry:
	li a0, 1
	li a1, 2
	li a2, 3
	li a3, 4
	li a4, 5
	li a5, 6
	li a6, 7
	li a7, 8
	call sum
	sw a0, 32(sp)
	lw t0, 32(sp)
	sw t0, 36(sp)
	li a0, 1
	li a1, 2
	li a2, 3
	li a3, 4
	li a4, 5
	li a5, 6
	li a6, 7
	li a7, 8
	li t0, 9
	sw t0, 0(sp)
	li t0, 10
	sw t0, 4(sp)
	li t0, 11
	sw t0, 8(sp)
	li t0, 12
	sw t0, 12(sp)
	li t0, 13
	sw t0, 16(sp)
	li t0, 14
	sw t0, 20(sp)
	li t0, 15
	sw t0, 24(sp)
	li t0, 16
	sw t0, 28(sp)
	call sum2
	sw a0, 40(sp)
	lw t0, 40(sp)
	sw t0, 44(sp)
	lw t0, 36(sp)
	sw t0, 48(sp)
	lw t0, 44(sp)
	sw t0, 52(sp)
	lw t0, 48(sp)
	lw t1, 52(sp)
	add t0, t0, t1
	sw t0, 56(sp)
	lw a0, 56(sp)
	lw ra, 60(sp)
	addi sp, sp, 64
	ret
