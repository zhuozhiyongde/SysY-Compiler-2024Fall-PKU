decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

global @b_0 = alloc [[i32, 2], 1], {{0, 0}}

fun @main(): i32 {
%entry:
	ret 5
}


