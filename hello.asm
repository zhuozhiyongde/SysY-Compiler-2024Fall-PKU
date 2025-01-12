decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

global @buf_0 = alloc [[[i32, 1], 3], 3], {{{1}, {0}, {2}}, {{0}, {0}, {0}}, {{0}, {0}, {0}}}
fun @main(): i32 {
%main_entry:
	ret 33
%jump_0:
	ret 0
}
