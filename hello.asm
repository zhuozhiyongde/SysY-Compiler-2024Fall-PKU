fun @main(): i32 {
%entry:
	@a_1 = alloc i32
	store 10, @a_1
	%0 = load @a_1
	%1 = gt %0, 1
	br %1, %then_0, %end_0
%then_0:
	%2 = load @a_1
	%3 = sub %2, 1
	store %3, @a_1
	@a_2 = alloc i32
	store 5, @a_2
	%4 = load @a_2
	%5 = lt %4, -1
	br %5, %then_1, %else_1
%then_1:
	ret 10
%else_1:
	@a_3 = alloc i32
	store 98, @a_3
	jump %end_1
%end_1:
	jump %end_0
%end_0:
	%6 = load @a_1
	%7 = eq %6, 9
	br %7, %then_2, %end_2
%then_2:
	%8 = load @a_1
	%9 = sub %8, 1
	@b_2 = alloc i32
	store %9, @b_2
	%10 = load @b_2
	%11 = sub %10, 1
	@a_2 = alloc i32
	store %11, @a_2
	%12 = load @a_2
	%13 = load @b_2
	%14 = ne %12, %13
	br %14, %then_3, %else_3
%then_3:
	%15 = load @a_2
	%16 = eq 0, %15
	br %16, %then_4, %end_4
%then_4:
	ret 0
%end_4:
	%17 = load @a_2
	ret %17
%else_3:
	%18 = load @b_2
	ret %18
%end_3:
	jump %end_2
%end_2:
	ret -1
}
