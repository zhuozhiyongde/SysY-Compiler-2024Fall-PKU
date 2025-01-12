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
	@a_2 = alloc [i32, 3]
	%0 = getelemptr @a_2, 0
	store 0, %0
	%1 = getelemptr @a_2, 1
	store 0, %1
	%2 = getelemptr @a_2, 2
	store 0, %2
	ret 3
%jump_0:
	ret 0
}
