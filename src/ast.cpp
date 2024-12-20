#include "include/ast.hpp"

int TEMP_COUNT = 0;
SymbolTable global_symbol_table;
SymbolTable* local_symbol_table = &global_symbol_table;

Result CompUnitAST::print() const {
  return func_def->print();
}

Result FuncDefAST::print() const {
  koopa_ofs << "fun @" << ident << "(): ";
  func_type->print();
  koopa_ofs << " {" << endl;
  koopa_ofs << "%entry:" << endl;
  block->print();
  koopa_ofs << "}" << endl;
  return Result();
}

Result FuncTypeAST::print() const {
  koopa_ofs << type;
  return Result();
}

Result BlockAST::print() const {
  SymbolTable* parent_symbol_table = local_symbol_table;
  local_symbol_table = new SymbolTable();
  local_symbol_table->set_parent(parent_symbol_table);
  for (auto& item : block_items) {
    if (!local_symbol_table->get_returned()) {
      item->print();
    }
  }
  delete local_symbol_table;
  local_symbol_table = parent_symbol_table;
  return Result();
}

Result ConstDeclAST::print() const {
  for (auto& item : const_defs) {
    item->print();
  }
  return Result();
}

Result ConstDefAST::print() const {
  string ident_with_suffix = local_symbol_table->assign(ident);
  Result value_result = value->print();
  local_symbol_table->create(ident_with_suffix, VAL_(value_result.value));
  return Result();
}

Result ConstInitValAST::print() const {
  return const_exp->print();
}

Result ConstExpAST::print() const {
  return exp->print();
}

Result VarDeclAST::print() const {
  for (auto& item : var_defs) {
    item->print();
  }
  return Result();
}

Result VarDefAST::print() const {
  string ident_with_suffix = local_symbol_table->assign(ident);
  if (value) {
    Result value_result = (*value)->print();
    koopa_ofs << "\t@" << ident_with_suffix << " = alloc i32" << endl;
    koopa_ofs << "\tstore " << value_result << ", @" << ident_with_suffix << endl;
    local_symbol_table->create(ident_with_suffix, VAR_);
  }
  else {
    koopa_ofs << "\t@" << ident_with_suffix << " = alloc i32" << endl;
    local_symbol_table->create(ident_with_suffix, VAR_);
  }
  return Result();
}

Result InitValAST::print() const {
  return exp->print();
}

Result StmtAssignAST::print() const {
  auto ident = ((LValAST*)l_val.get())->ident;
  string ident_with_suffix = local_symbol_table->locate(ident);
  assert(local_symbol_table->exist(ident_with_suffix));
  Result exp_result = exp->print();
  koopa_ofs << "\tstore " << exp_result << ", @" << ident_with_suffix << endl;
  return Result();
}

Result StmtExpAST::print() const {
  if (exp) {
    return (*exp)->print();
  }
  return Result();
}

Result StmtBlockAST::print() const {
  return block->print();
}

Result StmtReturnAST::print() const {
  if (exp) {
    Result exp_result = (*exp)->print();
    koopa_ofs << "\tret " << exp_result << endl;
  }
  else {
    koopa_ofs << "\tret" << endl;
  }
  local_symbol_table->set_returned(true);
  return Result();
}

Result LValAST::print() const {
  string ident_with_suffix = local_symbol_table->locate(ident);
  assert(local_symbol_table->exist(ident_with_suffix));
  auto symbol = local_symbol_table->read(ident_with_suffix);
  if (symbol.type == Symbol::Type::VAR) {
    Result result = REG_(TEMP_COUNT++);
    koopa_ofs << "\t" << result << " = load @" << ident_with_suffix << endl;
    return result;
  }
  else if (symbol.type == Symbol::Type::VAL) {
    return IMM_(symbol.value);
  }
  else {
    assert(false);
  }
}

Result ExpAST::print() const {
  return l_or_exp->print();
}

Result LOrExpAST::print() const {
  return l_and_exp->print();
}

Result LAndExpAST::print() const {
  return eq_exp->print();
}

Result LExpWithOpAST::print() const {
  Result lhs = left->print();
  Result rhs = right->print();
  if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
    switch (logical_op) {
    case LogicalOp::LOGICAL_OR:
      return IMM_(lhs.value || rhs.value);
    case LogicalOp::LOGICAL_AND:
      return IMM_(lhs.value && rhs.value);
    default:
      assert(false);
    }
  }
  else {
    Result result = REG_(TEMP_COUNT++);
    if (logical_op == LogicalOp::LOGICAL_OR) {
      Result temp = REG_(TEMP_COUNT++);
      koopa_ofs << "\t" << temp << " = or " << lhs << ", " << rhs << endl;
      koopa_ofs << "\t" << result << " = ne " << temp << ", 0" << endl;
    }
    else if (logical_op == LogicalOp::LOGICAL_AND) {
      Result temp_1 = REG_(TEMP_COUNT++);
      Result temp_2 = REG_(TEMP_COUNT++);
      koopa_ofs << "\t" << temp_1 << " = ne " << lhs << ", 0" << endl;
      koopa_ofs << "\t" << temp_2 << " = ne " << rhs << ", 0" << endl;
      koopa_ofs << "\t" << result << " = and " << temp_1 << ", " << temp_2 << endl;
    }
    return result;
  }
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

Result EqExpAST::print() const {
  return rel_exp->print();
}

Result EqExpWithOpAST::print() const {
  Result lhs = left->print();
  Result rhs = right->print();
  if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
    switch (eq_op) {
    case EqOp::EQ:
      return IMM_(lhs.value == rhs.value);
    case EqOp::NEQ:
      return IMM_(lhs.value != rhs.value);
    default:
      assert(false);
    }
  }
  else {
    Result result = REG_(TEMP_COUNT++);
    switch (eq_op) {
    case EqOp::EQ:
      koopa_ofs << "\t" << result << " = eq " << lhs << ", " << rhs << endl;
      break;
    case EqOp::NEQ:
      koopa_ofs << "\t" << result << " = ne " << lhs << ", " << rhs << endl;
      break;
    default:
      assert(false);
    }
    return result;
  }
}

Result RelExpAST::print() const {
  return add_exp->print();
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
  Result lhs = left->print();
  Result rhs = right->print();
  if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
    switch (rel_op) {
    case RelOp::LE:
      return IMM_(lhs.value <= rhs.value);
    case RelOp::GE:
      return IMM_(lhs.value >= rhs.value);
    case RelOp::LT:
      return IMM_(lhs.value < rhs.value);
    case RelOp::GT:
      return IMM_(lhs.value > rhs.value);
    default:
      assert(false);
    }
  }
  else {
    Result result = Result(Result::Type::REG, TEMP_COUNT++);
    if (rel_op == RelOp::LE) {
      koopa_ofs << "\t" << result << " = le " << lhs << ", " << rhs << endl;
    }
    else if (rel_op == RelOp::GE) {
      koopa_ofs << "\t" << result << " = ge " << lhs << ", " << rhs << endl;
    }
    else if (rel_op == RelOp::LT) {
      koopa_ofs << "\t" << result << " = lt " << lhs << ", " << rhs << endl;
    }
    else if (rel_op == RelOp::GT) {
      koopa_ofs << "\t" << result << " = gt " << lhs << ", " << rhs << endl;
    }
    return result;
  }
}

Result AddExpAST::print() const {
  return mul_exp->print();
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
  Result lhs = left->print();
  Result rhs = right->print();
  if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
    switch (add_op) {
    case AddOp::ADD:
      return IMM_(lhs.value + rhs.value);
    case AddOp::SUB:
      return IMM_(lhs.value - rhs.value);
    default:
      assert(false);
    }
  }
  else {
    Result result = Result(Result::Type::REG, TEMP_COUNT++);
    switch (add_op) {
    case AddOp::ADD:
      koopa_ofs << "\t" << result << " = add " << lhs << ", " << rhs << endl;
      break;
    case AddOp::SUB:
      koopa_ofs << "\t" << result << " = sub " << lhs << ", " << rhs << endl;
      break;
    default:
      assert(false);
    }
    return result;
  }
}

Result MulExpAST::print() const {
  return unary_exp->print();
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
  Result lhs = left->print();
  Result rhs = right->print();
  if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
    switch (mul_op) {
    case MulOp::MUL:
      return IMM_(lhs.value * rhs.value);
    case MulOp::DIV:
      return IMM_(lhs.value / rhs.value);
    case MulOp::MOD:
      return IMM_(lhs.value % rhs.value);
    default:
      assert(false);
    }
  }
  else {
    Result result = Result(Result::Type::REG, TEMP_COUNT++);
    switch (mul_op) {
    case MulOp::MUL:
      koopa_ofs << "\t" << result << " = mul " << lhs << ", " << rhs << endl;
      break;
    case MulOp::DIV:
      koopa_ofs << "\t" << result << " = div " << lhs << ", " << rhs << endl;
      break;
    case MulOp::MOD:
      koopa_ofs << "\t" << result << " = mod " << lhs << ", " << rhs << endl;
      break;
    default:
      assert(false);
    }
    return result;
  }
}

Result UnaryExpAST::print() const {
  return primary_exp->print();
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
  Result unary_exp_result = unary_exp->print();
  if (unary_exp_result.type == Result::Type::IMM) {
    switch (unary_op) {
    case UnaryOp::POSITIVE:
      return IMM_(unary_exp_result.value);
    case UnaryOp::NEGATIVE:
      return IMM_(-unary_exp_result.value);
    case UnaryOp::NOT:
      return IMM_(!unary_exp_result.value);
    default:
      assert(false);
    }
  }
  else {
    Result result = REG_(TEMP_COUNT++);
    switch (unary_op) {
    case UnaryOp::POSITIVE:
      koopa_ofs << "\t" << result << " = add 0, " << unary_exp_result << endl;
      break;
    case UnaryOp::NEGATIVE:
      koopa_ofs << "\t" << result << " = sub 0, " << unary_exp_result << endl;
      break;
    case UnaryOp::NOT:
      koopa_ofs << "\t" << result << " = eq 0, " << unary_exp_result << endl;
      break;
    default:
      assert(false);
    }
    return result;
  }
}


Result PrimaryExpAST::print() const {
  return exp->print();
}

Result PrimaryExpWithNumberAST::print() const {
  return IMM_(number);
}

Result PrimaryExpWithLValAST::print() const {
  return l_val->print();
}