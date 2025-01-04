fun @main(): i32 {
%entry:
	jump %while_entry_0
%while_entry_0:
	br 1, %while_body_0, %while_end_0
%while_body_0:
	@a_3 = alloc i32
	store 1, @a_3
	@b_3 = alloc i32
	store 2, @b_3
	jump %while_end_0
%jump_0:
	jump %while_entry_0
%while_end_0:
	ret -1
%jump_1:
	ret 0
}
