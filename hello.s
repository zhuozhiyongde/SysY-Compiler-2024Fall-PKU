	.text
	.globl main
main:
	li t0, 10
	li t1, 7
	slt t0, t1, t0
	snez t0, t0
	mv a0, t0
	ret
