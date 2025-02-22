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

extern ofstream koopa_ofs;
extern ofstream riscv_ofs;

void visit(const koopa_raw_program_t& program);
void visit(const koopa_raw_slice_t& slice);
void visit(const koopa_raw_function_t& func);
void visit(const koopa_raw_basic_block_t& bb);
void visit(const koopa_raw_value_t& value);
void visit(const koopa_raw_load_t& load, const koopa_raw_value_t& value);
void visit(const koopa_raw_store_t& store);
void visit(const koopa_raw_return_t& ret);
void visit(const koopa_raw_binary_t& binary, const koopa_raw_value_t& value);
void visit(const koopa_raw_branch_t& branch);
void visit(const koopa_raw_jump_t& jump);
