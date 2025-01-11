decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()


fun @main(): i32 {
%main_entry:
	@b_2 = alloc [i32, 4]
	%0 = getelemptr @b_2, 0
	store 0, %0
	%1 = getelemptr @b_2, 1
	store 1, %1
	%2 = getelemptr @b_2, 2
	store 0, %2
	%3 = getelemptr @b_2, 3
	store 0, %3

	@c_2 = alloc [i32, 7]
	%4 = getelemptr @c_2, 0
	store 2, %4
	%5 = getelemptr @c_2, 1
	store 8, %5
	%6 = getelemptr @c_2, 2
	store 6, %6
	%7 = getelemptr @c_2, 3
	store 3, %7
	%8 = getelemptr @c_2, 4
	store 9, %8
	%9 = getelemptr @c_2, 5
	store 1, %9
	%10 = getelemptr @c_2, 6
	store 5, %10

	@d_2 = alloc [i32, 11]

	@e_2 = alloc [i32, 2]
	%11 = getelemptr @e_2, 0
	store 22, %11
	%12 = getelemptr @e_2, 1
	store 33, %12

	@f_2 = alloc [i32, 6]

	@g_2 = alloc [i32, 9]
	%13 = getelemptr @g_2, 0
	store 85, %13
	%14 = getelemptr @g_2, 1
	store 0, %14
	%15 = getelemptr @g_2, 2
	store 1, %15
	%16 = getelemptr @g_2, 3
	store 29, %16
	%17 = getelemptr @g_2, 4
	store 0, %17
	%18 = getelemptr @g_2, 5
	store 0, %18
	%19 = getelemptr @g_2, 6
	store 0, %19
	%20 = getelemptr @g_2, 7
	store 0, %20
	%21 = getelemptr @g_2, 8
	store 0, %21

	ret 3
%jump_0:
	ret 0
}
