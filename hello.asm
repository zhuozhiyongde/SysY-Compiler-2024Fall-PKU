fun @main(): i32 {
%entry:
	@b_1 = alloc i32
	store 2, @b_1
	ret 1
%return_end_0:
	ret 0
}
