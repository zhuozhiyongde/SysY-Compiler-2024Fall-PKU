decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

global @a_0 = alloc i32, 0

fun @f(): i32 {
%entry:
	@a_2 = alloc i32
	store 1, @a_2
	ret 0
}

fun @main(): i32 {
%entry:
	@a_2 = alloc i32
	store 2, @a_2
	%0 = call @f()
	ret 0
%jump_0:
	ret 0
}
