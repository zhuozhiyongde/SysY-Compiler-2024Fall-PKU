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
lhs: 2
	ret 4
%jump_0:
	ret 0
}
