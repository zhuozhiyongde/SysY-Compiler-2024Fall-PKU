decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

global @ga_0 = alloc [i32, 10], zeroinit

global @gb_0 = alloc [[i32, 3], 2], {{5, 6, 7}, {8, 0, 0}}

fun @main(): i32 {
%entry:
	@a_2 = alloc [i32, 10]
	@b_2 = alloc [[i32, 3], 2]
	%0 = getelemptr @b_2, 0
	%1 = getelemptr %0, 0
	store 1, %1
	%2 = getelemptr %0, 1
	store 2, %2
	%3 = getelemptr %0, 2
	store 3, %3
	%4 = getelemptr @b_2, 1
	%5 = getelemptr %4, 0
	store 4, %5
	%6 = getelemptr %4, 1
	store 0, %6
	%7 = getelemptr %4, 2
	store 0, %7
	@sum_2 = alloc i32
	store 0, @sum_2
	@i_2 = alloc i32
	store 0, @i_2
	jump %while_0_entry
%while_0_entry:
	%8 = load @i_2
	%9 = lt %8, 2
	br %9, %while_0_body, %while_0_end
%while_0_body:
	@j_4 = alloc i32
	store 0, @j_4
	jump %while_1_entry
%while_1_entry:
	%10 = load @j_4
	%11 = lt %10, 3
	br %11, %while_1_body, %while_1_end
%while_1_body:
	%12 = load @sum_2
	%13 = load @i_2
	%14 = load @j_4
	%15 = getelemptr @b_2, %13
	%16 = getelemptr %15, %14
	%17 = load %16
	%18 = add %12, %17
	%19 = load @i_2
	%20 = load @j_4
	%21 = getelemptr @gb_0, %19
	%22 = getelemptr %21, %20
	%23 = load %22
	%24 = add %18, %23
	store %24, @sum_2
	%25 = load @j_4
	%26 = add %25, 1
	store %26, @j_4
	jump %while_1_entry
%while_1_end:
	%27 = load @i_2
	%28 = add %27, 1
	store %28, @i_2
	jump %while_0_entry
%while_0_end:
	store 0, @i_2
	jump %while_2_entry
%while_2_entry:
	%29 = load @i_2
	%30 = lt %29, 10
	br %30, %while_2_body, %while_2_end
%while_2_body:
	%31 = load @sum_2
	%32 = load @i_2
	%33 = add %31, %32
	%34 = load @i_2
	%35 = getelemptr @a_2, %34
	store %33, %35
	%36 = load @sum_2
	%37 = load @i_2
	%38 = add %36, %37
	%39 = load @i_2
	%40 = getelemptr @ga_0, %39
	store %38, %40
	%41 = load @i_2
	%42 = add %41, 1
	store %42, @i_2
	jump %while_2_entry
%while_2_end:
	%43 = load @sum_2
	ret %43
}


