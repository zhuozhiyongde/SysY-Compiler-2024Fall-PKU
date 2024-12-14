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
  return l_or_exp->print();
}

Result LOrExpAST::print() const {
  if (l_and_exp) {
    return (*l_and_exp)->print();
  }
  else if (l_exp_with_op) {
    return (*l_exp_with_op)->print();
  }
  return Result();
}

Result LAndExpAST::print() const {
  if (eq_exp) {
    return (*eq_exp)->print();
  }
  else if (l_exp_with_op) {
    return (*l_exp_with_op)->print();
  }
  return Result();
}

Result LExpWithOpAST::print() const {
  Result left_result = left->print();
  Result right_result = right->print();
  Result result = Result(Result::Type::REG, TEMP_COUNT++);
  if (logical_op == LogicalOp::LOGICAL_OR) {
    Result temp = Result(Result::Type::REG, TEMP_COUNT++);
    koopa_ofs << "\t" << temp << " = or " << left_result << ", " << right_result << endl;
    koopa_ofs << "\t" << result << " = ne " << temp << ", 0" << endl;
  }
  else if (logical_op == LogicalOp::LOGICAL_AND) {
    Result temp_1 = Result(Result::Type::REG, TEMP_COUNT++);
    Result temp_2 = Result(Result::Type::REG, TEMP_COUNT++);
    koopa_ofs << "\t" << temp_1 << " = ne " << left_result << ", 0" << endl;
    koopa_ofs << "\t" << temp_2 << " = ne " << right_result << ", 0" << endl;
    koopa_ofs << "\t" << result << " = and " << temp_1 << ", " << temp_2 << endl;
  }

  return result;
}

Result EqExpAST::print() const {
  if (rel_exp) {
    return (*rel_exp)->print();
  }
  else if (eq_exp_with_op) {
    return (*eq_exp_with_op)->print();
  }
  return Result();
}

EqExpWithOpAST::EqOp EqExpWithOpAST::convert(const string& op) const {
  if (op == "==") {
    return EqOp::EQ;
  }
  else if (op == "!=") {
    return EqOp::NEQ;
  }
  throw runtime_error("Invalid operator: " + op);
}

Result EqExpWithOpAST::print() const {
  Result left_result = left->print();
  Result right_result = right->print();
  Result result = Result(Result::Type::REG, TEMP_COUNT++);
  if (eq_op == EqOp::EQ) {
    koopa_ofs << "\t" << result << " = eq " << left_result << ", " << right_result << endl;
  }
  else if (eq_op == EqOp::NEQ) {
    koopa_ofs << "\t" << result << " = ne " << left_result << ", " << right_result << endl;
  }
  return result;
}

Result RelExpAST::print() const {
  if (add_exp) {
    return (*add_exp)->print();
  }
  else if (rel_exp_with_op) {
    return (*rel_exp_with_op)->print();
  }
  return Result();
}

RelExpWithOpAST::RelOp RelExpWithOpAST::convert(const string& op) const {
  if (op == "<=") {
    return RelOp::LE;
  }
  else if (op == ">=") {
    return RelOp::GE;
  }
  else if (op == "<") {
    return RelOp::LT;
  }
  else if (op == ">") {
    return RelOp::GT;
  }
  throw runtime_error("Invalid operator: " + op);
}

Result RelExpWithOpAST::print() const {
  Result left_result = left->print();
  Result right_result = right->print();
  Result result = Result(Result::Type::REG, TEMP_COUNT++);
  if (rel_op == RelOp::LE) {
    koopa_ofs << "\t" << result << " = le " << left_result << ", " << right_result << endl;
  }
  else if (rel_op == RelOp::GE) {
    koopa_ofs << "\t" << result << " = ge " << left_result << ", " << right_result << endl;
  }
  else if (rel_op == RelOp::LT) {
    koopa_ofs << "\t" << result << " = lt " << left_result << ", " << right_result << endl;
  }
  else if (rel_op == RelOp::GT) {
    koopa_ofs << "\t" << result << " = gt " << left_result << ", " << right_result << endl;
  }
  return result;
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

AddExpWithOpAST::AddOp AddExpWithOpAST::convert(const string& op) const {
  if (op == "+") {
    return AddOp::ADD;
  }
  else if (op == "-") {
    return AddOp::SUB;
  }
  throw runtime_error("Invalid operator: " + op);
}

Result AddExpWithOpAST::print() const {
  Result left_result = left->print();
  Result right_result = right->print();
  Result result = Result(Result::Type::REG, TEMP_COUNT++);
  if (add_op == AddOp::ADD) {
    koopa_ofs << "\t" << result << " = add " << left_result << ", " << right_result << endl;
  }
  else if (add_op == AddOp::SUB) {
    koopa_ofs << "\t" << result << " = sub " << left_result << ", " << right_result << endl;
  }
  return result;
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

MulExpWithOpAST::MulOp MulExpWithOpAST::convert(const string& op) const {
  if (op == "*") {
    return MulOp::MUL;
  }
  else if (op == "/") {
    return MulOp::DIV;
  }
  else if (op == "%") {
    return MulOp::MOD;
  }
  throw runtime_error("Invalid operator: " + op);
}

Result MulExpWithOpAST::print() const {
  Result left_result = left->print();
  Result right_result = right->print();
  Result result = Result(Result::Type::REG, TEMP_COUNT++);
  if (mul_op == MulOp::MUL) {
    koopa_ofs << "\t" << result << " = mul " << left_result << ", " << right_result << endl;
  }
  else if (mul_op == MulOp::DIV) {
    koopa_ofs << "\t" << result << " = div " << left_result << ", " << right_result << endl;
  }
  else if (mul_op == MulOp::MOD) {
    koopa_ofs << "\t" << result << " = mod " << left_result << ", " << right_result << endl;
  }
  return result;
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

UnaryExpWithOpAST::UnaryOp UnaryExpWithOpAST::convert(const string& op) const {
  if (op == "+") {
    return UnaryOp::POSITIVE;
  }
  else if (op == "-") {
    return UnaryOp::NEGATIVE;
  }
  else if (op == "!") {
    return UnaryOp::NOT;
  }
  throw runtime_error("Invalid operator: " + op);
}

Result UnaryExpWithOpAST::print() const {
  Result result = Result(Result::Type::REG, TEMP_COUNT++);
  if (unary_op == UnaryOp::POSITIVE) {
    Result unary_exp_result = unary_exp->print();
    koopa_ofs << "\t" << result << " = add 0, " << unary_exp_result << endl;
  }
  else if (unary_op == UnaryOp::NEGATIVE) {
    Result unary_exp_result = unary_exp->print();
    koopa_ofs << "\t" << result << " = sub 0, " << unary_exp_result << endl;
  }
  else if (unary_op == UnaryOp::NOT) {
    Result unary_exp_result = unary_exp->print();
    koopa_ofs << "\t" << result << " = eq 0, " << unary_exp_result << endl;
  }
  return result;
}
