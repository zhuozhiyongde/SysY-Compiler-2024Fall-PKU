fun @main(): i32 {
%entry:
	@a_1 = alloc i32
	store 10, @a_1
	%0 = load @a_1
	%1 = gt %0, 1
	br %1, %then_0, %end_0
%then_0:
	%2 = load @a_1
	%3 = gt %2, 2
	br %3, %then_1, %end_1
%then_1:
	%4 = load @a_1
	%5 = lt %4, 3
	br %5, %then_2, %else_2
%then_2:
	%6 = load @a_1
	ret %6
%return_end_0:
	jump %end_2
%else_2:
	%7 = load @a_1
	%8 = gt %7, 4
	br %8, %then_3, %end_3
%then_3:
	%9 = load @a_1
	%10 = lt %9, 5
	br %10, %then_4, %else_4
%then_4:
	%11 = load @a_1
	%12 = add %11, 1
	ret %12
%return_end_1:
	jump %end_4
%else_4:
	%13 = load @a_1
	%14 = add %13, 2
	ret %14
%return_end_2:
	jump %end_4
%end_4:
	jump %end_3
%end_3:
	jump %end_2
%end_2:
	jump %end_1
%end_1:
	jump %end_0
%end_0:
	ret -1
%return_end_3:
	ret 0
}
