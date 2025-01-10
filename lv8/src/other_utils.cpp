#include "include/other_utils.hpp"

string koopaRawValueTagToString(int tag) {
    switch (tag) {
    case koopa_raw_value_tag_t::KOOPA_RVT_INTEGER:
        return "KOOPA_RVT_INTEGER";
    case koopa_raw_value_tag_t::KOOPA_RVT_ZERO_INIT:
        return "KOOPA_RVT_ZERO_INIT";
    case koopa_raw_value_tag_t::KOOPA_RVT_UNDEF:
        return "KOOPA_RVT_UNDEF";
    case koopa_raw_value_tag_t::KOOPA_RVT_AGGREGATE:
        return "KOOPA_RVT_AGGREGATE";
    case koopa_raw_value_tag_t::KOOPA_RVT_FUNC_ARG_REF:
        return "KOOPA_RVT_FUNC_ARG_REF";
    case koopa_raw_value_tag_t::KOOPA_RVT_BLOCK_ARG_REF:
        return "KOOPA_RVT_BLOCK_ARG_REF";
    case koopa_raw_value_tag_t::KOOPA_RVT_ALLOC:
        return "KOOPA_RVT_ALLOC";
    case koopa_raw_value_tag_t::KOOPA_RVT_GLOBAL_ALLOC:
        return "KOOPA_RVT_GLOBAL_ALLOC";
    case koopa_raw_value_tag_t::KOOPA_RVT_LOAD:
        return "KOOPA_RVT_LOAD";
    case koopa_raw_value_tag_t::KOOPA_RVT_STORE:
        return "KOOPA_RVT_STORE";
    case koopa_raw_value_tag_t::KOOPA_RVT_GET_PTR:
        return "KOOPA_RVT_GET_PTR";
    case koopa_raw_value_tag_t::KOOPA_RVT_GET_ELEM_PTR:
        return "KOOPA_RVT_GET_ELEM_PTR";
    case koopa_raw_value_tag_t::KOOPA_RVT_BINARY:
        return "KOOPA_RVT_BINARY";
    case koopa_raw_value_tag_t::KOOPA_RVT_BRANCH:
        return "KOOPA_RVT_BRANCH";
    case koopa_raw_value_tag_t::KOOPA_RVT_JUMP:
        return "KOOPA_RVT_JUMP";
    case koopa_raw_value_tag_t::KOOPA_RVT_CALL:
        return "KOOPA_RVT_CALL";
    case koopa_raw_value_tag_t::KOOPA_RVT_RETURN:
        return "KOOPA_RVT_RETURN";
    default:
        return "UNKNOWN_TAG";
    }
}


string koopaRawBinaryOpToString(int op) {
    switch (op) {
    case koopa_raw_binary_op_t::KOOPA_RBO_EQ:
        return "KOOPA_RBO_EQ";
    case koopa_raw_binary_op_t::KOOPA_RBO_NOT_EQ:
        return "KOOPA_RBO_NOT_EQ";
    case koopa_raw_binary_op_t::KOOPA_RBO_GT:
        return "KOOPA_RBO_GT";
    case koopa_raw_binary_op_t::KOOPA_RBO_LT:
        return "KOOPA_RBO_LT";
    case koopa_raw_binary_op_t::KOOPA_RBO_GE:
        return "KOOPA_RBO_GE";
    case koopa_raw_binary_op_t::KOOPA_RBO_LE:
        return "KOOPA_RBO_LE";
    case koopa_raw_binary_op_t::KOOPA_RBO_ADD:
        return "KOOPA_RBO_ADD";
    case koopa_raw_binary_op_t::KOOPA_RBO_SUB:
        return "KOOPA_RBO_SUB";
    case koopa_raw_binary_op_t::KOOPA_RBO_MUL:
        return "KOOPA_RBO_MUL";
    case koopa_raw_binary_op_t::KOOPA_RBO_DIV:
        return "KOOPA_RBO_DIV";
    case koopa_raw_binary_op_t::KOOPA_RBO_MOD:
        return "KOOPA_RBO_MOD";
    case koopa_raw_binary_op_t::KOOPA_RBO_AND:
        return "KOOPA_RBO_AND";
    case koopa_raw_binary_op_t::KOOPA_RBO_OR:
        return "KOOPA_RBO_OR";
    case koopa_raw_binary_op_t::KOOPA_RBO_XOR:
        return "KOOPA_RBO_XOR";
    case koopa_raw_binary_op_t::KOOPA_RBO_SHL:
        return "KOOPA_RBO_SHL";
    case koopa_raw_binary_op_t::KOOPA_RBO_SHR:
        return "KOOPA_RBO_SHR";
    case koopa_raw_binary_op_t::KOOPA_RBO_SAR:
        return "KOOPA_RBO_SAR";
    default:
        return "UNKNOWN_OP";
    }
}

string koopaRawSliceKindItemToString(int kind) {
    switch (kind) {
    case koopa_raw_slice_item_kind_t::KOOPA_RSIK_FUNCTION:
        return "KOOPA_RSIK_PROGRAM";
    case koopa_raw_slice_item_kind_t::KOOPA_RSIK_BASIC_BLOCK:
        return "KOOPA_RSIK_BASIC_BLOCK";
    case koopa_raw_slice_item_kind_t::KOOPA_RSIK_VALUE:
        return "KOOPA_RSIK_VALUE";
    default:
        return "UNKNOWN_KIND";
    }
}