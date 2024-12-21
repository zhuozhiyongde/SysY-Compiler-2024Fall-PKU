fun @main(): i32 {
%entry:
	br 0, %then_0, %end_0
%then_0:
	ret 1
%return_end_0:
	jump %end_0
%end_0:
	br 2, %then_1, %else_1
%then_1:
	ret 1
%return_end_1:
	jump %end_1
%else_1:
	ret 2
%return_end_2:
	jump %end_1
%end_1:
	ret 0
}
