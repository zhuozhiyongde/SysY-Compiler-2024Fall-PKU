fun @main(): i32 {
%entry:
	br 0, %then_0, %end_0
%then_0:
	ret 1
%return_end_0:
	jump %end_0
%end_0:
	ret 1
%return_end_1:
	ret 0
}
