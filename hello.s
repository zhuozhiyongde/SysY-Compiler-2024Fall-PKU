	.text
	.globl main
main:
	li t0, -8000
	add sp, sp, t0
entry:
	li t0, 10
	sw t0, 2040(sp) // a_1
	lw t0, 2040(sp)
	sw t0, 2044(sp)
	lw t0, 2044(sp)
	li t1, 1
	sgt t0, t0, t1
	li t2, 2048
	add t2, sp, t2
	sw t0, (t2)
	li t1, 2048
	add t1, sp, t1
	lw t0, (t1)
	bnez t0, then_0
	beqz t0, end_0
then_0:
	lw t0, 2040(sp)
	li t0, 2052
	add t0, sp, t0
	sw t0, (t0)
	li t1, 2052
	add t1, sp, t1
	lw t0, (t1)
	li t2, 2
	sgt t0, t0, t2
	li t3, 2056
	add t3, sp, t3
	sw t0, (t3)
	li t1, 2056
	add t1, sp, t1
	lw t0, (t1)
	bnez t0, then_1
	beqz t0, end_1
end_0:
	li a0, -1
	li t0, 8000
	add sp, sp, t0
	ret
then_1:
	lw t0, 2040(sp)
	li t0, 2060
	add t0, sp, t0
	sw t0, (t0)
	li t1, 2060
	add t1, sp, t1
	lw t0, (t1)
	li t2, 3
	slt t0, t0, t2
	li t3, 2064
	add t3, sp, t3
	sw t0, (t3)
	li t1, 2064
	add t1, sp, t1
	lw t0, (t1)
	bnez t0, then_2
	beqz t0, else_2
end_1:
	j end_0
then_2:
	lw t0, 2040(sp)
	li t0, 2068
	add t0, sp, t0
	sw t0, (t0)
	li t0, 2068
	add t0, sp, t0
	lw a0, (t0)
	li t1, 8000
	add sp, sp, t1
	ret
else_2:
	lw t0, 2040(sp)
	li t0, 2072
	add t0, sp, t0
	sw t0, (t0)
	li t1, 2072
	add t1, sp, t1
	lw t0, (t1)
	li t2, 4
	sgt t0, t0, t2
	li t3, 2076
	add t3, sp, t3
	sw t0, (t3)
	li t1, 2076
	add t1, sp, t1
	lw t0, (t1)
	bnez t0, then_3
	beqz t0, end_3
then_3:
	lw t0, 2040(sp)
	li t0, 2080
	add t0, sp, t0
	sw t0, (t0)
	li t1, 2080
	add t1, sp, t1
	lw t0, (t1)
	li t2, 5
	slt t0, t0, t2
	li t3, 2084
	add t3, sp, t3
	sw t0, (t3)
	li t1, 2084
	add t1, sp, t1
	lw t0, (t1)
	bnez t0, then_4
	beqz t0, else_4
end_3:
	j end_2
then_4:
	lw t0, 2040(sp)
	li t0, 2088
	add t0, sp, t0
	sw t0, (t0)
	li t1, 2088
	add t1, sp, t1
	lw t0, (t1)
	li t2, 1
	add t0, t0, t2
	li t3, 2092
	add t3, sp, t3
	sw t0, (t3)
	li t0, 2092
	add t0, sp, t0
	lw a0, (t0)
	li t1, 8000
	add sp, sp, t1
	ret
else_4:
	lw t0, 2040(sp)
	li t0, 2096
	add t0, sp, t0
	sw t0, (t0)
	li t1, 2096
	add t1, sp, t1
	lw t0, (t1)
	li t2, 2
	add t0, t0, t2
	li t3, 2100
	add t3, sp, t3
	sw t0, (t3)
	li t0, 2100
	add t0, sp, t0
	lw a0, (t0)
	li t1, 8000
	add sp, sp, t1
	ret
end_2:
	j end_1
