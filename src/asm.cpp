#include "include/asm.hpp"

// Riscv 辅助类，用于生成 riscv 汇编代码
Riscv riscv;
// 全局 context 管理器
ContextManager context_manager;
// 当前函数对应的 context
Context context;
// 寄存器管理器
RegisterManager register_manager;

/**
 * @brief 翻译 Koopa IR 程序
 * @param[in] program 程序
 */
void visit(const koopa_raw_program_t& program) {
	// 翻译所有全局变量
	visit(program.values);
	// 翻译所有函数
	visit(program.funcs);
};

/**
 * @brief 翻译 Koopa IR 切片
 * @param[in] slice 切片
 */
void visit(const koopa_raw_slice_t& slice) {
	// 切片 slice 是存储一系列元素的数组
	// 遍历数组，对每个元素进行翻译
	for (size_t i = 0; i < slice.len; ++i) {
		auto ptr = slice.buffer[i];
		// 根据 slice 的 kind 决定将 ptr 视作何种元素
		// RSIK: Raw Slice Kind, 区分 slice 的类型
		switch (slice.kind) {
		case KOOPA_RSIK_FUNCTION:
			// 访问函数
			// 先强制转换为 koopa_raw_function_t 类型
			visit(reinterpret_cast<koopa_raw_function_t>(ptr));
			break;
		case KOOPA_RSIK_BASIC_BLOCK:
			// 访问基本块
			// 先强制转换为 koopa_raw_basic_block_t 类型
			visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
			break;
		case KOOPA_RSIK_VALUE:
			// 访问指令
			// 先强制转换为 koopa_raw_value_t 类型
			visit(reinterpret_cast<koopa_raw_value_t>(ptr));
			break;
		default:
			// 我们暂时不会遇到其他内容, 于是不对其做任何处理
			cout << slice.kind << endl;
			assert(false);
		}
	}
};

/**
 * @brief 翻译 Koopa IR 函数
 * @param[in] func 函数
 */
void visit(const koopa_raw_function_t& func) {
	// 忽略库函数
	if (func->bbs.len == 0) {
		return;
	}
	// 访问所有基本块，bbs: basic block slice
	riscv_ofs << endl;
	riscv._text();
	// 忽略函数名前的@，@main -> main
	riscv._globl(func->name + 1);
	riscv._label(func->name + 1);
	// 计算栈帧大小，此时还只是指令计数，没乘 4
	int cnt = 0;
	// 计算函数体内 call 指令最多用到的参数个数
	int stack_args = 0;
	// 计算函数体内 alloc 指令们总共分配的空间大小
	int alloc_size = 0;
	// 判断函数体内是否有 call 指令，若有，则需要多分配一条 store 指令来压栈代表返回值的 ra 寄存器
	bool has_call = false;
	// 遍历所有基本块
	for (size_t i = 0; i < func->bbs.len; ++i) {
		auto bb = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
		// 计算基本块内指令数
		cnt += bb->insts.len;
		// 遍历基本块内所有指令
		for (size_t j = 0; j < bb->insts.len; ++j) {
			auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
			// 如果是 unit 类型，则不占用栈帧空间
			if (inst->ty->tag == KOOPA_RTT_UNIT) {
				cnt -= 1;
			}
			// 如果是 call 指令，需要额外计算变量表所需空间
			if (inst->kind.tag == KOOPA_RVT_CALL) {
				int args = inst->kind.data.call.args.len;
				// 取最大值，不同 call 之间可以覆写
				stack_args = max(stack_args, args - 8);
				has_call = true;
			}
			// 如果是 alloc 指令，检查是否为数组
			if (inst->kind.tag == KOOPA_RVT_ALLOC) {
				int size = get_alloc_size(inst->ty->data.pointer.base);
				alloc_size += size;
			}
		}
	}
	// 如果函数体内有 call 指令，需要多分配一条 store 指令来存储 ra 寄存器
	cnt += has_call;
	// 额外分配压栈参数空间
	cnt += stack_args;
	// 乘 4，转换为实际字节数
	cnt *= 4;
	// 加上 alloc 指令分配的空间
	cnt += alloc_size;
	// 对齐到 16 的倍数
	cnt = (cnt + 15) / 16 * 16;
	// [DEBUG]：检查是否超过 imm12 的限制
	// cnt = 8000;
	context_manager.create_context(func->name + 1, cnt);
	riscv._addi("sp", "sp", -cnt);
	context = context_manager.get_context(func->name + 1);
	// [DEBUG]：检查是否超过 imm12 的限制
	// context.stack_used = 2040;
	// 如果函数体内有 call 指令，则需要存储代表返回后下一条指令地址的 ra 寄存器到栈底
	if (has_call) {
		riscv._sw("ra", "sp", context.stack_size - 4);
		context.save_ra = true;
	}
	// 在栈顶先分配掉压栈参数所需空间
	context.stack_used = stack_args * 4;
	// 访问所有基本块
	visit(func->bbs);
};

/**
 * @brief 翻译 Koopa IR 基本块
 * @param[in] bb 基本块
 */
void visit(const koopa_raw_basic_block_t& bb) {
	// 输出基本块标号
	riscv_ofs << bb->name + 1 << ":" << endl;
	// 访问所有指令，insts: instruction slice
	visit(bb->insts);
};

/**
 * @brief 计算 alloc 指令分配的空间大小，对于数组可以递归处理
 * @param[in] ty 类型
 * @return 分配的空间大小
 */
int get_alloc_size(const koopa_raw_type_t ty) {
	switch (ty->tag) {
		// 空类型不占用空间
	case KOOPA_RTT_UNIT:
		return 0;
		// 函数类型不占用空间
	case KOOPA_RTT_FUNCTION:
		return 0;
		// 32 位整数占用 4 字节
	case KOOPA_RTT_INT32:
		return 4;
		// 指针类型占用 4 字节
	case KOOPA_RTT_POINTER:
		return 4;
		// 数组类型占用空间为数组长度乘以数组元素类型占用空间
	case KOOPA_RTT_ARRAY:
		return ty->data.array.len * get_alloc_size(ty->data.array.base);
	default:
		printf("Invalid type: %s\n", koopaRawTypeTagToString(ty->tag).c_str());
		assert(false);
	}
}

/**
 * @brief 翻译 alloc 指令，设置已用帧栈，记录数组的栈偏移
 * @param[in] value 指令
 */
void alloc(const koopa_raw_value_t& value) {
	// [DEBUG]：输出 alloc 指令的名称和类型
	// printf("alloc name: %s\n", value->name);
	// printf("alloc rtt: %s\n", koopaRawTypeTagToString(value->ty->tag).c_str());
	// 计算 alloc 指令分配的空间大小
	int size = get_alloc_size(value->ty->data.pointer.base);
	// [DEBUG]：输出 alloc 指令分配的空间大小
	// printf("alloc size: %d\n", size);
	// 记录栈偏移
	context.stack_map[value] = context.stack_used;
	// 更新已用栈帧
	context.stack_used += size;
}

/**
 * @brief 翻译值，一般就是翻译指令
 * @param[in] value 值
 */
void visit(const koopa_raw_value_t& value) {
	// 根据值（指令）的类型判断后续需要如何访问
	const auto& kind = value->kind;
	// RVT: Raw Value Tag, 区分指令类型
	register_manager.reset();
	// [DEBUG]：输出指令类型  
	// printf("visit: %s\n", koopaRawValueTagToString(kind.tag).c_str());
	// [DEBUG]
	// riscv_ofs << "---" << endl;
	// riscv_ofs << "[" << koopaRawValueTagToString(kind.tag).c_str() << "]" << endl;
	// 说明：什么时候传 value，什么时候不传
	// 1. 如果需要存储单条值（指令）的计算结果，则需要传入 value
	// 2. 否则不传入 value
	switch (kind.tag) {
	case KOOPA_RVT_RETURN:
		// 访问 return 指令
		visit(kind.data.ret);
		break;
	case KOOPA_RVT_BINARY:
		// 处理 binary 指令（双目运算）
		// 由于要在栈上存储单条指令的计算结果，所以将指令本身 value 也传入
		// 用于后续在 symbol_map 中存储结果
		visit(kind.data.binary, value);
		break;
	case KOOPA_RVT_ALLOC:
		// 处理 alloc 指令（局部分配）
		alloc(value);
		break;
	case KOOPA_RVT_GLOBAL_ALLOC:
		// 处理 global_alloc 指令（全局分配）
		visit(kind.data.global_alloc, value);
		break;
	case KOOPA_RVT_AGGREGATE:
		// 处理 aggregate 指令（初始化列表）
		visit(kind.data.aggregate);
		break;
	case KOOPA_RVT_GET_PTR:
		// 处理 get_ptr 指令（指针计算）
		visit(kind.data.get_ptr, value);
		break;
	case KOOPA_RVT_GET_ELEM_PTR:
		// 处理 get_elem_ptr 指令（元素指针计算）
		visit(kind.data.get_elem_ptr, value);
		break;
	case KOOPA_RVT_STORE:
		// 处理 store 指令（存储）
		visit(kind.data.store);
		break;
	case KOOPA_RVT_LOAD:
		// 处理 load 指令（加载）
		// 由于需要在栈上记录加载结果所在的位置，所以需要传入 value
		visit(kind.data.load, value);
		break;
	case KOOPA_RVT_BRANCH:
		// 处理 branch 指令（条件分支）
		visit(kind.data.branch);
		break;
	case KOOPA_RVT_JUMP:
		// 处理 jump 指令（无条件跳转）
		visit(kind.data.jump);
		break;
	case KOOPA_RVT_CALL:
		// 处理 call 指令（函数调用）
		// 由于需要判断是否需要存储返回值，所以需要传入 value
		visit(kind.data.call, value);
		break;
	default:
		// 其他类型暂时遇不到
		printf("Invalid instruction: %s\n", koopaRawValueTagToString(kind.tag).c_str());
		assert(false);
	}
};

/**
 * @brief 处理全局分配指令
 * @param[in] global_alloc 全局分配指令的数据
 * @param[in] value 全局分配指令自身，全局存储时用作键
 */
void visit(const koopa_raw_global_alloc_t& global_alloc, const koopa_raw_value_t& value) {
	riscv_ofs << endl;
	// 在全局变量表中创建全局变量
	context_manager.create_global(value);
	// 获取全局变量名
	auto global_name = context_manager.get_global(value);
	// 输出 .data .global label 等格式信息
	riscv._data();
	riscv._globl(global_name);
	riscv._label(global_name);
	// 判断初始化值的类型
	auto init = global_alloc.init;
	switch (init->kind.tag) {
	case KOOPA_RVT_INTEGER:
		// 输出整数
		riscv._word(init->kind.data.integer.value);
		break;
	case KOOPA_RVT_ZERO_INIT:
		// 输出 0 初始化，指定个数
		riscv._zero(get_alloc_size(init->ty));
		break;
	case KOOPA_RVT_AGGREGATE:
		// 递归处理初始化列表
		visit(init->kind.data.aggregate);
		break;
	default:
		// 其他类型暂时遇不到
		printf("Invalid global_alloc init: %s\n", koopaRawValueTagToString(init->kind.tag).c_str());
		assert(false);
	}
}

/**
 * @brief 处理初始化列表
 * @param[in] aggregate 初始化列表的数据
 */
void visit(const koopa_raw_aggregate_t& aggregate) {
	// 遍历初始化列表
	for (int i = 0; i < aggregate.elems.len; i++) {
		// 获取列表中的当前元素
		auto elem = reinterpret_cast<koopa_raw_value_t>(aggregate.elems.buffer[i]);
		// 处理整数
		if (elem->kind.tag == KOOPA_RVT_INTEGER) {
			riscv._word(elem->kind.data.integer.value);
		}
		// 递归处理下一级初始化列表 aggregate
		else {
			visit(elem);
		}
	}
}

/**
 * @brief 处理 getptr 指针计算指令，会存放加载出来的指针到栈上
 * @param[in] get_ptr getptr 指针计算指令的数据
 * @param[in] value getptr 指针计算指令自身，存储时用作键
 */
void visit(const koopa_raw_get_ptr_t& get_ptr, const koopa_raw_value_t& value) {
	// [DEBUG]
	// printf("get_ptr src: %s\n", koopaRawValueTagToString(get_ptr.src->kind.tag).c_str());
	// printf("get_ptr index: %s\n", koopaRawValueTagToString(get_ptr.index->kind.tag).c_str());
	// riscv_ofs << "get_ptr src: " << koopaRawValueTagToString(get_ptr.src->kind.tag).c_str() << endl;
	// riscv_ofs << "get_ptr index: " << koopaRawValueTagToString(get_ptr.index->kind.tag).c_str() << endl;
	// 获取栈偏移
	auto offset = context.stack_map[get_ptr.src];
	// 准备存放基准值的寄存器
	auto base = register_manager.new_reg();
	// 判断是基准值的来源
	switch (get_ptr.src->kind.tag) {
		// 函数参数传进来的，已经被提前 load 后压栈到 offset(sp)
	case KOOPA_RVT_LOAD:
		riscv._lw(base, "sp", offset);
		break;
		// 全局变量的指针，使用 la 指令获取地址
	case KOOPA_RVT_GLOBAL_ALLOC:
		riscv._la(base, context_manager.get_global(get_ptr.src));
		break;
	default:
		assert(false);
	}
	// 判断 index 是否为非零，如果是零的话就不用加上偏移了
	bool is_non_zero = register_manager.get_operand_reg(get_ptr.index);
	// 非零，则需要加上偏移
	if (is_non_zero) {
		// 获取 index 所在的寄存器
		auto bias = register_manager.reg_map[get_ptr.index];
		// 获取存放步长的临时寄存器
		auto step = register_manager.tmp_reg();
		// 要获取步长，就需要获取数组元素的类型
		auto size = get_alloc_size(get_ptr.src->ty->data.pointer.base);
		// 判断步长是否为 2 的幂次
		auto power = is_power_of_two(size);
		// 如果是 2 的幂次，则可以进行强度削减，转换为左移指令
		if (power != -1) {
			riscv._li(step, power);
			riscv._sll(bias, bias, step);
		}
		// 否则，还是使用乘法指令
		else {
			riscv._li(step, size);
			riscv._mul(bias, bias, step);
		}
		// 计算最终地址
		riscv._add(base, base, bias);
	}
	// 将加载出来的指针存到栈上
	riscv._sw(base, "sp", context.stack_used);
	// 必须先存再压栈，不然 context.stack_used 会变
	context.push(value, context.stack_used);
}

/**
 * @brief 处理 getelemptr 指针指令，会存放加载出来的指针到栈上
 * @param[in] get_elem_ptr getelemptr 指针计算指令的数据
 * @param[in] value getelemptr 指针计算指令自身，存储时用作键
 */
void visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr, const koopa_raw_value_t& value) {
	// [DEBUG]
	// printf("get_elem_ptr src: %s\n", koopaRawValueTagToString(get_elem_ptr.src->kind.tag).c_str());
	// printf("get_elem_ptr index: %s\n", koopaRawValueTagToString(get_elem_ptr.index->kind.tag).c_str());
	// riscv_ofs << "get_elem_ptr src: " << koopaRawValueTagToString(get_elem_ptr.src->kind.tag).c_str() << endl;
	// riscv_ofs << "get_elem_ptr index: " << koopaRawValueTagToString(get_elem_ptr.index->kind.tag).c_str() << endl;
	// 获取栈偏移
	auto offset = context.stack_map[get_elem_ptr.src];
	// 准备存放基准值的寄存器
	auto base = register_manager.new_reg();
	// 判断是基准值的来源
	switch (get_elem_ptr.src->kind.tag) {
		// 全局变量的指针，使用 la 指令获取地址
	case KOOPA_RVT_GLOBAL_ALLOC:
		riscv._la(base, context_manager.get_global(get_elem_ptr.src));
		break;
		// 一个之前的 getelemptr 指令，结果已经存到栈上
	case KOOPA_RVT_GET_ELEM_PTR:
		riscv._lw(base, "sp", offset);
		break;
		// 一个之前的 getptr 指令，结果已经存到栈上
	case KOOPA_RVT_GET_PTR:
		riscv._lw(base, "sp", offset);
		break;
		// 一个之前的 alloc 指令，起始位置已经在 alloc 函数中存到了栈上
	case KOOPA_RVT_ALLOC:
		riscv._addi(base, "sp", offset);
		break;
	default:
		assert(false);
	}
	// 判断 index 是否为非零，如果是零的话就不用加上偏移了
	bool is_non_zero = register_manager.get_operand_reg(get_elem_ptr.index);
	// 非零，则需要加上偏移
	if (is_non_zero) {
		// 获取 index 所在的寄存器
		auto bias = register_manager.reg_map[get_elem_ptr.index];
		// 获取存放步长的临时寄存器
		auto step = register_manager.tmp_reg();
		// 要获取步长，就需要获取数组元素的类型
		auto size = get_alloc_size(get_elem_ptr.src->ty->data.pointer.base->data.array.base);
		// 判断步长是否为 2 的幂次
		auto power = is_power_of_two(size);
		// 如果是 2 的幂次，则可以进行强度削减，转换为左移指令
		if (power != -1) {
			riscv._li(step, power);
			riscv._sll(bias, bias, step);
		}
		// 否则，还是使用乘法指令
		else {
			riscv._li(step, size);
			riscv._mul(bias, bias, step);
		}
		// 计算最终地址
		riscv._add(base, base, bias);
	}
	// 将加载出来的指针存到栈上
	riscv._sw(base, "sp", context.stack_used);
	// 必须先存再压栈，不然 context.stack_used 会变
	context.push(value, context.stack_used);
}

/**
 * @brief 处理 call 指令，如果有返回值，会将返回值存到栈上
 * @param[in] call call 指令的数据
 * @param[in] value call 指令自身，存储时用作键
 */
void visit(const koopa_raw_call_t& call, const koopa_raw_value_t& value) {
	// 获取参数个数
	int args = call.args.len;
	// 处理前 8 个参数
	for (int i = 0; i < min(args, 8); i++) {
		// 取出正在处理的参数的 value
		auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
		// 获取存放到的目的地寄存器
		// 这 8 个参数一定是存到 a0 - a7 寄存器中的
		auto target = "a" + to_string(i);
		// 若为整数，则直接将整数存到目标寄存器中
		if (arg->kind.tag == KOOPA_RVT_INTEGER) {
			riscv._li(target, arg->kind.data.integer.value);
		}
		// 若为指针参数，则从栈上获取
		else if (arg->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
			riscv._lw(target, "sp", context.stack_map[arg]);
		}
		// 若为之前某操作的中间结果，实际上也必然为整数，但是需要先加载
		else {
			riscv._lw(target, "sp", context.stack_map[arg]);
		}
	}
	// 处理超过 8 个参数的情况，此时需要将参数存到栈上
	for (int i = 8; i < args; i++) {
		// 取出正在处理的参数的 value
		auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
		// 计算栈偏移
		int target = (i - 8) * 4;
		// 若为整数，则直接将整数存到目标寄存器中
		if (arg->kind.tag == KOOPA_RVT_INTEGER) {
			// 准备一个临时寄存器
			auto tmp = register_manager.new_reg();
			// 将整数存到临时寄存器中
			riscv._li(tmp, arg->kind.data.integer.value);
			// 将临时寄存器里的整数存到栈上目标地址
			riscv._sw(tmp, "sp", target);
		}
		// 若为指针参数，则从栈上获取
		else if (arg->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
			// 准备一个临时寄存器
			auto tmp = register_manager.new_reg();
			// 从栈上获取指针到临时寄存器中
			riscv._lw(tmp, "sp", context.stack_map[arg]);
			// 将临时寄存器里的指针存到栈上目标地址
			riscv._sw(tmp, "sp", target);
		}
		// 为之前某操作的中间结果，实际上也必然为整数，但是需要先加载
		else {
			// 准备一个临时寄存器
			auto tmp = register_manager.new_reg();
			// 从栈上获取整数到临时寄存器中
			riscv._lw(tmp, "sp", context.stack_map[arg]);
			// 将临时寄存器里的整数存到栈上目标地址
			riscv._sw(tmp, "sp", target);
		}
		// 释放临时寄存器，必须在循环间释放
		register_manager.reset();
	}
	// 调用函数
	riscv._call(call.callee->name + 1);
	// 判断是否需要存储返回值
	if (value->ty->tag != KOOPA_RTT_UNIT) {
		// 将返回值存到栈上
		context.push(value, context.stack_used);
		// 注意要先压栈才能通过 stack_map 访问到
		riscv._sw("a0", "sp", context.stack_map[value]);
	}
}

/**
 * @brief 处理 branch 指令，根据条件跳转到不同的分支
 * @param[in] branch branch 指令的数据
 */
void visit(const koopa_raw_branch_t& branch) {
	// 准备条件表达式
	register_manager.get_operand_reg(branch.cond);
	// 获取条件表达式所在的寄存器
	auto cond = register_manager.reg_map[branch.cond];
	// 根据条件跳转到不同的基本块，输出的是基本块的 label
	riscv._bnez(cond, branch.true_bb->name + 1);
	riscv._beqz(cond, branch.false_bb->name + 1);
}

/**
 * @brief 处理 jump 指令，跳转到目标基本块
 * @param[in] jump jump 指令的数据
 */
void visit(const koopa_raw_jump_t& jump) {
	// 跳转到目标基本块
	riscv._jump(jump.target->name + 1);
}

/**
 * @brief 处理 load 指令，将加载出来的值存到栈上
 * @param[in] load load 指令的数据
 * @param[in] value load 指令自身，存储时用作键
 */
void visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value) {
	// 准备一个临时寄存器
	auto reg = register_manager.new_reg();
	// 获取当前栈偏移，压栈加载出来的值时需要用到
	auto bias = context.stack_used;
	// 打印加载的值的类型
	// [DEBUG]
	// printf("load: %s\n", koopaRawValueTagToString(load.src->kind.tag).c_str());
	// riscv_ofs << "load: " << koopaRawValueTagToString(load.src->kind.tag).c_str() << endl;
	// 如果是全局变量，需要先获取地址，再解引用获取值
	if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
		riscv._la(reg, context_manager.get_global(load.src));
		riscv._lw(reg, reg, 0);
	}
	// 如果是指针，加载出来后还需要解引用一下
	else if (load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
		riscv._lw(reg, "sp", context.stack_map[load.src]);
		riscv._lw(reg, reg, 0);
	}
	// 对于 get_ptr 指令同理
	else if (load.src->kind.tag == KOOPA_RVT_GET_PTR) {
		riscv._lw(reg, "sp", context.stack_map[load.src]);
		riscv._lw(reg, reg, 0);
	}
	// 如果是栈上变量，直接获取值
	else {
		riscv._lw(reg, "sp", context.stack_map[load.src]);
	}
	// 将加载出来的值存到栈上
	riscv._sw(reg, "sp", bias);
	context.push(value, bias);
}

/**
 * @brief 处理 store 指令
 * @param[in] store store 指令的数据
 */
void visit(const koopa_raw_store_t& store) {
	// [DEBUG]
	// 打印要存储的值和目标地址的类型
	// printf("store value: %s\n", koopaRawValueTagToString(store.value->kind.tag).c_str());
	// printf("store dest: %s\n", koopaRawValueTagToString(store.dest->kind.tag).c_str());
	// 准备要存储的值
	register_manager.get_operand_reg(store.value);
	// 如果是全局变量，需要先获取地址，再存储到解引用后的位置上
	if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
		auto reg = register_manager.new_reg();
		riscv._la(reg, context_manager.get_global(store.dest));
		riscv._sw(register_manager.reg_map[store.value], reg, 0);
	}
	// 如果是指针，也需要再解引用一下才能获得目标地址
	else if (store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
		auto reg = register_manager.new_reg();
		riscv._lw(reg, "sp", context.stack_map[store.dest]);
		riscv._sw(register_manager.reg_map[store.value], reg, 0);
	}
	// 对于 get_ptr 指令同理
	else if (store.dest->kind.tag == KOOPA_RVT_GET_PTR) {
		auto reg = register_manager.new_reg();
		riscv._lw(reg, "sp", context.stack_map[store.dest]);
		riscv._sw(register_manager.reg_map[store.value], reg, 0);
	}
	// 如果是栈上变量，直接存储到栈上目标位置即可
	else {
		assert(register_manager.reg_map[store.value] != "");
		riscv._sw(register_manager.reg_map[store.value], "sp", context.stack_map[store.dest]);
	}
}

/**
 * @brief 处理 return 指令，如果有返回值，会将返回值存到 a0 寄存器中
 * @param[in] ret return 指令的数据
 */
void visit(const koopa_raw_return_t& ret) {
	// 如果返回值非空，那么先把值搞到 a0 寄存器中
	if (ret.value != nullptr) {
		// 打印返回值的类型
		// [DEBUG]
		// printf("return: %s\n", koopaRawValueTagToString(ret.value->kind.tag).c_str());
		// 判断返回值的类型
		switch (ret.value->kind.tag) {
			// 形如 ret 1 直接返回整数的
		case KOOPA_RVT_INTEGER:
			riscv._li("a0", ret.value->kind.data.integer.value);
			break;
			// 形如 ret %n, 返回之前某操作的中间结果
		case KOOPA_RVT_BINARY:
		case KOOPA_RVT_LOAD:
		case KOOPA_RVT_CALL:
			riscv._lw("a0", "sp", context.stack_map[ret.value]);
			break;
		default:
			assert(false && "Invalid return value");
		}
	}
	// 即将返回，需要恢复代表返回地址（返回后下一条指令地址）的寄存器 ra
	if (context.save_ra) {
		riscv._lw("ra", "sp", context.stack_size - 4);
	}
	// 恢复栈指针
	riscv._addi("sp", "sp", context.stack_size);
	// 返回
	riscv._ret();
};

/**
 * @brief 处理 binary 指令，计算二元运算的结果，并存到栈上
 * @param[in] binary binary 指令的数据
 * @param[in] value binary 指令自身，存储时用作键
 */
void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value) {
	// 准备两个操作数到寄存器中
	bool lhs_use_reg = register_manager.get_operand_reg(binary.lhs);
	bool rhs_use_reg = register_manager.get_operand_reg(binary.rhs);
	// 确定中间结果的寄存器
	// 如果两个都是 0，即在 x0 寄存器，显然要新开一个寄存器，来存储中间结果
	if (!lhs_use_reg && !rhs_use_reg) {
		register_manager.reg_map[value] = register_manager.new_reg();
	}
	// 对于其他情况，找一个已有寄存器来存储中间结果
	else if (lhs_use_reg) {
		register_manager.reg_map[value] = register_manager.reg_map[binary.lhs];
	}
	else {
		register_manager.reg_map[value] = register_manager.reg_map[binary.rhs];
	}

	// 获取存放中间结果的寄存器
	const auto cur = register_manager.reg_map[value];
	// 获取两个操作数所在的寄存器
	const auto lhs = register_manager.reg_map[binary.lhs];
	const auto rhs = register_manager.reg_map[binary.rhs];
	// 根据二元运算符的类型，执行相应的指令
	switch (binary.op) {
	case KOOPA_RBO_EQ:
		riscv._xor(cur, lhs, rhs);
		riscv._seqz(cur, cur);
		break;
	case KOOPA_RBO_NOT_EQ:
		riscv._xor(cur, lhs, rhs);
		riscv._snez(cur, cur);
		break;
	case KOOPA_RBO_LE:
		// lhs <= rhs 等价于 !(lhs > rhs)
		riscv._sgt(cur, lhs, rhs);
		riscv._seqz(cur, cur);
		break;
	case KOOPA_RBO_GE:
		// lhs >= rhs 等价于 !(lhs < rhs)
		riscv._slt(cur, lhs, rhs);
		riscv._seqz(cur, cur);
		break;
	case KOOPA_RBO_LT:
		riscv._slt(cur, lhs, rhs);
		break;
	case KOOPA_RBO_GT:
		riscv._sgt(cur, lhs, rhs);
		break;
	case KOOPA_RBO_OR:
		riscv._or(cur, lhs, rhs);
		break;
	case KOOPA_RBO_AND:
		riscv._and(cur, lhs, rhs);
		break;
	case KOOPA_RBO_SUB:
		riscv._sub(cur, lhs, rhs);
		break;
	case KOOPA_RBO_ADD:
		riscv._add(cur, lhs, rhs);
		break;
	case KOOPA_RBO_MUL:
		riscv._mul(cur, lhs, rhs);
		break;
	case KOOPA_RBO_DIV:
		riscv._div(cur, lhs, rhs);
		break;
	case KOOPA_RBO_MOD:
		riscv._rem(cur, lhs, rhs);
		break;
	default:
		printf("Invalid binary operation: %s\n", koopaRawBinaryOpToString(binary.op).c_str());
	}
	// 把结果存回栈中
	context.push(value, context.stack_used);
	// 注意要先压栈才能通过 stack_map 访问到
	riscv._sw(cur, "sp", context.stack_map[value]);
}
