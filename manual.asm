fun @main(): i32 {
%entry:
	@a_1 = alloc i32
	store 0, @a_1
	@c_1 = alloc i32
	store 1, @c_1 // 1
	@d_1 = alloc i32
	store 2, @d_1 // 2
	%0 = load @a_1 // 0
	%short_result_0 = alloc i32
	br %0, %short_true_0, %short_false_0 // j false
%short_true_0:
	store 1, %short_result_0
	jump %short_end_0
%short_false_0:
	%1 = eq 0, 0
	%2 = ne %1, 0 // 0
	store %2, %short_result_0
	jump %short_end_0
%short_end_0:
	%3 = load %short_result_0 // 0
	br %3, %then_0, %else_0 // j else
%then_0:
	store 3, @c_1 // 3
	jump %end_0
%else_0:
	store 3, @d_1 // 3
	@a_3 = alloc i32
	store 1, @a_3 // 1
	%4 = load @a_3 // 1
	%short_result_1 = alloc i32
	br %4, %short_true_1, %short_false_1 // j true
%short_true_1:
	store 1, %short_result_1 // 1
	jump %short_end_1
%short_false_1:
	%5 = eq 0, 0
	%6 = ne %5, 0
	store %6, %short_result_1
	jump %short_end_1
%short_end_1:
	%7 = load %short_result_1 // 1
	br %7, %then_1, %else_1 // j then
%then_1:
	store 4, @c_1 // 4
	jump %end_1
%else_1:
	store 4, @d_1
	jump %end_1
%end_1:
	%8 = load @a_3 // 1
	%9 = eq %8, 0 // 0
	br %9, %then_2, %else_2 // j else
%then_2:
	ret 1
%return_end_0:
	jump %end_2
%else_2:
	%10 = load @a_3 // 1
	%11 = eq %10, 0 // 0
	%short_result_2 = alloc i32
	br %11, %short_true_2, %short_false_2 // false
%short_false_2:
	store 0, %short_result_2 // 0
	jump %short_end_2
%short_true_2:
	%12 = load @a_3
	%13 = eq %12, -1
	%14 = ne %11, 0
	%15 = ne %13, 0
	%16 = and %14, %15
	store %16, %short_result_2
	jump %short_end_2
%short_end_2:
	%17 = load %short_result_2 // 0
	br %17, %then_3, %end_3 // j end
%then_3:
	ret 2
%return_end_1:
	jump %end_3
%end_3:
	jump %end_2
%end_2:
	jump %end_0
%end_0:
	%18 = load @a_1 // 0
	%19 = add %18, 0 // 0
	%20 = load @c_1 // 4
	%21 = add %19, %20 // 3
	%22 = load @d_1 // 3
	%23 = add %21, %22 // 5
	ret %23
%return_end_2:
	ret 0
}
