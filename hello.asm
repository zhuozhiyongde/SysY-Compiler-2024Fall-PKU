decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

global @a_0 = alloc i32, 1

fun @main(): i32 {
%main_entry:
	%0 = load @a_0
	ret %0
%jump_0:
	ret 0
}
