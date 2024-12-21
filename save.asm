decl @getint(): i32
decl @getch(): i32
decl @getarray(*i32): i32
decl @putint(i32)
decl @putch(i32)
decl @putarray(i32, *i32)
decl @starttime()
decl @stoptime()

fun @main(): i32 {
%entry_main:
	@a_1 = alloc i32
	store 0, @a_1
	@c_1 = alloc i32
	store 1, @c_1
	@d_1 = alloc i32
	store 2, @d_1
	%0 = load @a_1
	@result1 = alloc i32
	br %0, %then1, %else1
%then1:
	store 1, @result1
	jump %end1
%else1:
	store 0, @result1
	jump %end1
%end1:
	%1 = load @result1
	br %1, %then2, %else2
%then2:
	store 3, @c_1
	jump %if_end2
%else2:
	store 3, @d_1
	@a_3 = alloc i32
	store 1, @a_3
	%2 = load @a_3
	@result3 = alloc i32
	br %2, %then3, %else3
%then3:
	store 1, @result3
	jump %end3
%else3:
	store 0, @result3
	jump %end3
%end3:
	%3 = load @result3
	br %3, %then4, %else4
%then4:
	store 4, @c_1
	jump %if_end4
%else4:
	store 4, @d_1
	jump %if_end4
%if_end4:
	%4 = load @a_3
	%5 = eq %4, 0
	br %5, %then5, %else5
%then5:
	ret 1
%ret_label1:
	jump %if_end5
%else5:
	%6 = load @a_3
	%7 = eq %6, 0
	@result6 = alloc i32
	br %7, %then6, %else6
%then6:
	%8 = load @a_3
	%9 = eq %8, -1
	%10 = ne %9, 0
	store %10, @result6
	jump %end6
%else6:
	store 0, @result6
	jump %end6
%end6:
	%11 = load @result6
	br %11, %then7, %if_end7
%then7:
	ret 2
%ret_label2:
	jump %if_end7
%if_end7:
	jump %if_end5
%if_end5:
	jump %if_end2
%if_end2:
	%12 = load @a_1
	%13 = add %12, 0
	%14 = load @c_1
	%15 = add %13, %14
	%16 = load @d_1
	%17 = add %15, %16
	ret %17
%ret_label3:
	ret 114514

}

