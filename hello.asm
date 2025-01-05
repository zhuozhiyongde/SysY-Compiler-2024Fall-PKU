decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

global @x_0 = alloc i32, zeroinit
global @init_0 = alloc i32, 1

fun @main(): i32 {
%entry:
	%0 = load @x_0
	call @putint(%0)
	call @putch(32)
	call @putint(10)
	call @putch(32)
	call @putint(11)
	call @putch(32)
	%1 = load @init_0
	call @putint(%1)
	call @putch(10)
	ret 0
%jump_0:
	ret 0
}
