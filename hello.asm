
fun @sum(@a0: i32, @a1: i32, @a2: i32, @a3: i32, @a4: i32, @a5: i32, @a6: i32, @a7: i32): i32 {
%sum_entry:
	@a0_1 = alloc i32
	store @a0, @a0_1
	@a1_1 = alloc i32
	store @a1, @a1_1
	@a2_1 = alloc i32
	store @a2, @a2_1
	@a3_1 = alloc i32
	store @a3, @a3_1
	@a4_1 = alloc i32
	store @a4, @a4_1
	@a5_1 = alloc i32
	store @a5, @a5_1
	@a6_1 = alloc i32
	store @a6, @a6_1
	@a7_1 = alloc i32
	store @a7, @a7_1
	%0 = load @a0_1
	%1 = load @a1_1
	%2 = add %0, %1
	%3 = load @a2_1
	%4 = add %2, %3
	%5 = load @a3_1
	%6 = add %4, %5
	%7 = load @a4_1
	%8 = add %6, %7
	%9 = load @a5_1
	%10 = add %8, %9
	%11 = load @a6_1
	%12 = add %10, %11
	%13 = load @a7_1
	%14 = add %12, %13
	ret %14
%jump_0:
	ret 0
}

fun @sum2(@a0: i32, @a1: i32, @a2: i32, @a3: i32, @a4: i32, @a5: i32, @a6: i32, @a7: i32, @a8: i32, @a9: i32, @a10: i32, @a11: i32, @a12: i32, @a13: i32, @a14: i32, @a15: i32): i32 {
%sum2_entry:
	@a0_1 = alloc i32
	store @a0, @a0_1
	@a1_1 = alloc i32
	store @a1, @a1_1
	@a2_1 = alloc i32
	store @a2, @a2_1
	@a3_1 = alloc i32
	store @a3, @a3_1
	@a4_1 = alloc i32
	store @a4, @a4_1
	@a5_1 = alloc i32
	store @a5, @a5_1
	@a6_1 = alloc i32
	store @a6, @a6_1
	@a7_1 = alloc i32
	store @a7, @a7_1
	@a8_1 = alloc i32
	store @a8, @a8_1
	@a9_1 = alloc i32
	store @a9, @a9_1
	@a10_1 = alloc i32
	store @a10, @a10_1
	@a11_1 = alloc i32
	store @a11, @a11_1
	@a12_1 = alloc i32
	store @a12, @a12_1
	@a13_1 = alloc i32
	store @a13, @a13_1
	@a14_1 = alloc i32
	store @a14, @a14_1
	@a15_1 = alloc i32
	store @a15, @a15_1
	%15 = load @a0_1
	%16 = load @a1_1
	%17 = add %15, %16
	%18 = load @a2_1
	%19 = add %17, %18
	%20 = load @a3_1
	%21 = add %19, %20
	%22 = load @a4_1
	%23 = add %21, %22
	%24 = load @a5_1
	%25 = add %23, %24
	%26 = load @a6_1
	%27 = add %25, %26
	%28 = load @a7_1
	%29 = add %27, %28
	%30 = load @a8_1
	%31 = add %29, %30
	%32 = load @a9_1
	%33 = add %31, %32
	%34 = load @a10_1
	%35 = add %33, %34
	%36 = load @a11_1
	%37 = add %35, %36
	%38 = load @a12_1
	%39 = add %37, %38
	%40 = load @a13_1
	%41 = add %39, %40
	%42 = load @a14_1
	%43 = add %41, %42
	%44 = load @a15_1
	%45 = add %43, %44
	ret %45
%jump_1:
	ret 0
}

fun @main(): i32 {
%main_entry:
	@x_2 = alloc i32
	%46 = call @sum(1, 2, 3, 4, 5, 6, 7, 8)
	store %46, @x_2
	@y_2 = alloc i32
	%47 = call @sum2(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)
	store %47, @y_2
	%48 = load @x_2
	%49 = load @y_2
	%50 = add %48, %49
	ret %50
%jump_2:
	ret 0
}
