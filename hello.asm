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
	@a_2 = alloc i32
	store 1, @a_2
	@b_2 = alloc i32
	store 2, @b_2
	@c_2 = alloc i32
	%0 = load @a_2
	%1 = load @b_2
	%2 = add %0, %1
	store %2, @c_2
	%3 = load @c_2
	call @putint(%3)
	call @putch(10)
	%4 = load @c_2
	ret %4
%jump_0:
	ret 0
}
