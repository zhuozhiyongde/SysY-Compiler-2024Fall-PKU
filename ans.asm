decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

global @buf_0 = alloc [[i32, 100], 2], zeroinit

fun @main(): i32 {
%entry:
	@i_2 = alloc i32
	store 0, @i_2
	@k_2 = alloc i32
	store 0, @k_2
	%0 = load @i_2
	%1 = getelemptr @buf_0, 0
	%2 = getelemptr %1, %0
	%3 = load %2
	%4 = load @k_2
	%5 = getelemptr @buf_0, 1
	%6 = getelemptr %5, %4
	store %3, %6
	ret 33
}


