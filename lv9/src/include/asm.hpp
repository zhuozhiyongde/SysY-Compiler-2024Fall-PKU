#pragma once

#include "koopa.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "include/backend_utils.hpp"
#include "include/other_utils.hpp"

using namespace std;

// 文件读写流的全局变量

extern ofstream koopa_ofs;
extern ofstream riscv_ofs;

// 不会存储值到栈上的指令

void visit(const koopa_raw_program_t& program);
void visit(const koopa_raw_slice_t& slice);
void visit(const koopa_raw_function_t& func);
void visit(const koopa_raw_basic_block_t& bb);
void visit(const koopa_raw_value_t& value);
void visit(const koopa_raw_aggregate_t& aggregate);
void visit(const koopa_raw_store_t& store);
void visit(const koopa_raw_return_t& ret);
void visit(const koopa_raw_branch_t& branch);
void visit(const koopa_raw_jump_t& jump);

// 会存储值到栈上的指令

void visit(const koopa_raw_global_alloc_t& global_alloc, const koopa_raw_value_t& value);
void visit(const koopa_raw_get_ptr_t& get_ptr, const koopa_raw_value_t& value);
void visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr, const koopa_raw_value_t& value);
void visit(const koopa_raw_call_t& call, const koopa_raw_value_t& value);
void visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value);
void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value);

// 没有数据体，但是会分配栈空间

void alloc(const koopa_raw_value_t& value);

// 获取 value 所占用空间大小的辅助函数

int get_alloc_size(const koopa_raw_type_t ty);