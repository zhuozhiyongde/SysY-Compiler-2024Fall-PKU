#include "include/ast.hpp"

int TEMP_COUNT = 0;

Result CompUnitAST::print() const {
  return func_def->print();
}

Result FuncDefAST::print() const {
  koopa_ofs << "fun @" << ident << "(): ";
  func_type->print();
  koopa_ofs << " {" << endl;
  block->print();
  koopa_ofs << "}" << endl;
  return Result();
}

Result FuncTypeAST::print() const {
  koopa_ofs << type;
  return Result();
}

Result BlockAST::print() const {
  koopa_ofs << "%entry:" << endl;
  stmt->print();
  return Result();
}

Result StmtAST::print() const {
  Result result = exp->print();
  koopa_ofs << "\tret " << result;
  koopa_ofs << endl;
  return Result();
}

Result ExpAST::print() const {
  return add_exp->print();
}

Result AddExpAST::print() const {
  if (mul_exp) {
    return (*mul_exp)->print();
  }
  else if (add_exp_with_op) {
    return (*add_exp_with_op)->print();
  }
  return Result();
}

Result AddExpWithOpAST::print() const {
  Result left_result = left->print();
  Result right_result = right->print();
  if (add_op == AddOp::ADD) {
    koopa_ofs << "\t%" << TEMP_COUNT++ << " = add " << left_result << ", " << right_result << endl;
  }
  else if (add_op == AddOp::SUB) {
    koopa_ofs << "\t%" << TEMP_COUNT++ << " = sub " << left_result << ", " << right_result << endl;
  }
  return Result(Result::Type::REG, TEMP_COUNT - 1);
}

Result MulExpAST::print() const {
  if (unary_exp) {
    return (*unary_exp)->print();
  }
  else if (mul_exp_with_op) {
    return (*mul_exp_with_op)->print();
  }
  return Result();
}

Result MulExpWithOpAST::print() const {
  Result left_result = left->print();
  Result right_result = right->print();
  if (mul_op == MulOp::MUL) {
    koopa_ofs << "\t%" << TEMP_COUNT++ << " = mul " << left_result << ", " << right_result << endl;
  }
  else if (mul_op == MulOp::DIV) {
    koopa_ofs << "\t%" << TEMP_COUNT++ << " = div " << left_result << ", " << right_result << endl;
  }
  else if (mul_op == MulOp::MOD) {
    koopa_ofs << "\t%" << TEMP_COUNT++ << " = mod " << left_result << ", " << right_result << endl;
  }
  return Result(Result::Type::REG, TEMP_COUNT - 1);
}

Result UnaryExpAST::print() const {
  if (primary_exp) {
    return (*primary_exp)->print();
  }
  else if (unary_exp_with_op) {
    return (*unary_exp_with_op)->print();
  }
  return Result();
}

Result PrimaryExpAST::print() const {
  if (exp) {
    return (*exp)->print();
  }
  else if (number) {
    Result result = Result(Result::Type::IMM, *number);
    return result;
  }
  return Result();
}

Result UnaryExpWithOpAST::print() const {
  if (unary_op == UnaryOp::POSITIVE) {
    Result result = unary_exp->print();
    koopa_ofs << "\t%" << TEMP_COUNT++ << " = add 0, " << result << endl;
  }
  else if (unary_op == UnaryOp::NEGATIVE) {
    Result result = unary_exp->print();
    koopa_ofs << "\t%" << TEMP_COUNT++ << " = sub 0, " << result << endl;
  }
  else if (unary_op == UnaryOp::NOT) {
    Result result = unary_exp->print();
    koopa_ofs << "\t%" << TEMP_COUNT++ << " = eq 0, " << result << endl;
  }
  return Result(Result::Type::REG, TEMP_COUNT - 1);
}
