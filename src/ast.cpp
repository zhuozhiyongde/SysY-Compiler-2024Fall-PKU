#include "include/ast.hpp"

SymbolTable global_symbol_table;
SymbolTable* local_symbol_table = &global_symbol_table;
FrontendContextManager fcm;

Result CompUnitAST::print() const {
  return func_def->print();
}

Result FuncDefAST::print() const {
  koopa_ofs << "fun @" << ident << "(): ";
  func_type->print();
  koopa_ofs << " {" << endl;
  koopa_ofs << "%entry:" << endl;
  block->print();
  koopa_ofs << "\tret 0" << endl;
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
    if (!local_symbol_table->is_returned) {
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
  // 在当前层级符号表中分配常量名
  string ident_with_suffix = local_symbol_table->assign(ident);
  // 计算常量值
  Result value_result = value->print();
  // 在当前层级符号表中创建常量
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
  // 在当前层级符号表中分配变量
  string ident_with_suffix = local_symbol_table->assign(ident);
  // 判断是否需要生成 alloc 指令
  if (!fcm.is_symbol_allocated[ident_with_suffix]) {
    koopa_ofs << "\t@" << ident_with_suffix << " = alloc i32" << endl;
    fcm.is_symbol_allocated[ident_with_suffix] = true;
  }
  // 若初始值不为空，则生成 store 指令
  if (value) {
    Result value_result = (*value)->print();
    koopa_ofs << "\tstore " << value_result << ", @" << ident_with_suffix << endl;
  }
  // 在当前层级符号表中创建变量
  local_symbol_table->create(ident_with_suffix, VAR_);
  return Result();
}

Result InitValAST::print() const {
  return exp->print();
}

Result StmtIfAST::print() const {
  // 先计算表达式结果
  Result exp_result = exp->print();
  // 创建新的符号表，避免 if 语句中的单句 return 修改当前块 is_returned 
  SymbolTable* parent_symbol_table = local_symbol_table;
  local_symbol_table = new SymbolTable();
  local_symbol_table->set_parent(parent_symbol_table);
  // 准备标签
  string then_label = fcm.get_then_label();
  string else_label = fcm.get_else_label();
  string end_label = fcm.get_end_label();
  fcm.add_if_else_count();
  // 根据是否存在 else 语句进行分支处理
  if (else_stmt) {
    koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << else_label << endl;
    koopa_ofs << then_label << ":" << endl;
    then_stmt->print();
    koopa_ofs << "\tjump " << end_label << endl;
    koopa_ofs << else_label << ":" << endl;
    (*else_stmt)->print();
    koopa_ofs << "\tjump " << end_label << endl;
  }
  else {
    koopa_ofs << "\tbr " << exp_result << ", " << then_label << ", " << end_label << endl;
    koopa_ofs << then_label << ":" << endl;
    then_stmt->print();
    koopa_ofs << "\tjump " << end_label << endl;
  }
  koopa_ofs << end_label << ":" << endl;
  // 恢复符号表
  delete local_symbol_table;
  local_symbol_table = parent_symbol_table;

  return Result();
}

Result StmtAssignAST::print() const {
  // 获取变量名
  auto ident = ((LValAST*)l_val.get())->ident;
  // 获取变量名在符号表中的位置，可能需要向上级符号表查找
  string ident_with_suffix = local_symbol_table->locate(ident);
  assert(local_symbol_table->exist(ident_with_suffix));
  // 计算表达式结果并存储到变量中
  Result exp_result = exp->print();
  koopa_ofs << "\tstore " << exp_result << ", @" << ident_with_suffix << endl;
  return Result();
}

Result StmtExpAST::print() const {
  // 若表达式不为空，则计算表达式结果
  if (exp) {
    return (*exp)->print();
  }
  return Result();
}

Result StmtReturnAST::print() const {
  // 若表达式不为空，则计算表达式结果
  if (exp) {
    Result exp_result = (*exp)->print();
    koopa_ofs << "\tret " << exp_result << endl;
  }
  // 若表达式为空，则单句返回即可
  else {
    koopa_ofs << "\tret" << endl;
  }
  // 设置当前块 is_returned 为 true
  local_symbol_table->is_returned = true;
  // 设置返回结束标签，这样可以避免一个标号末尾出现多句 ret / br / jump 的情况
  auto ret_end_label = fcm.get_ret_end_label();
  koopa_ofs << ret_end_label << ":" << endl;
  return Result();
}

Result LValAST::print() const {
  // 获取变量名在符号表中的位置，然后读取其值
  string ident_with_suffix = local_symbol_table->locate(ident);
  assert(local_symbol_table->exist(ident_with_suffix));
  auto symbol = local_symbol_table->read(ident_with_suffix);
  // 若变量是变量，则使用 load 指令读取其值
  if (symbol.type == Symbol::Type::VAR) {
    Result result = NEW_REG_(fcm);
    koopa_ofs << "\t" << result << " = load @" << ident_with_suffix << endl;
    return result;
  }
  // 若变量是常量，则直接返回常量值
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
  // 先计算左表达式结果
  Result lhs = left->print();
  // 短路求值
  if (logical_op == LogicalOp::LOGICAL_OR) {
    if (lhs.type == Result::Type::IMM && lhs.value != 0) {
      return IMM_(1);
    }
  }
  else if (logical_op == LogicalOp::LOGICAL_AND) {
    if (lhs.type == Result::Type::IMM && lhs.value == 0) {
      return IMM_(0);
    }
  }
  // 其他情况必然需要计算右表达式结果
  Result rhs = right->print();
  // 若左右表达式结果均为常量，则直接返回常量结果
  if (lhs.type == Result::Type::IMM && rhs.type == Result::Type::IMM) {
    // 由于已经排除了短路求值的情况，所以返回结果必然是右表达式结果
    return IMM_(rhs.value);
  }
  // 若左右表达式结果不均为常量，则使用临时变量计算结果
  else {
    Result result = NEW_REG_(fcm);
    if (logical_op == LogicalOp::LOGICAL_OR) {
      Result temp = NEW_REG_(fcm);
      koopa_ofs << "\t" << temp << " = or " << lhs << ", " << rhs << endl;
      koopa_ofs << "\t" << result << " = ne " << temp << ", 0" << endl;
    }
    else if (logical_op == LogicalOp::LOGICAL_AND) {
      Result temp_1 = NEW_REG_(fcm);
      Result temp_2 = NEW_REG_(fcm);
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
  // 先计算左右表达式结果
  Result lhs = left->print();
  Result rhs = right->print();
  // 若左右表达式结果均为常量，则直接返回常量结果
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
  // 若左右表达式结果不均为常量，则使用临时变量计算结果
  else {
    Result result = NEW_REG_(fcm);
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
  // 先计算左右表达式结果
  Result lhs = left->print();
  Result rhs = right->print();
  // 若左右表达式结果均为常量，则直接返回常量结果
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
  // 若左右表达式结果不均为常量，则使用临时变量计算结果
  else {
    Result result = Result(Result::Type::REG, fcm.get_temp_count());
    switch (rel_op) {
    case RelOp::LE:
      koopa_ofs << "\t" << result << " = le " << lhs << ", " << rhs << endl;
      break;
    case RelOp::GE:
      koopa_ofs << "\t" << result << " = ge " << lhs << ", " << rhs << endl;
      break;
    case RelOp::LT:
      koopa_ofs << "\t" << result << " = lt " << lhs << ", " << rhs << endl;
      break;
    case RelOp::GT:
      koopa_ofs << "\t" << result << " = gt " << lhs << ", " << rhs << endl;
      break;
    default:
      assert(false);
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
  // 先计算左右表达式结果
  Result lhs = left->print();
  Result rhs = right->print();
  // 若左右表达式结果均为常量，则直接返回常量结果
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
  // 若左右表达式结果不均为常量，则使用临时变量计算结果
  else {
    Result result = Result(Result::Type::REG, fcm.get_temp_count());
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
  // 先计算左右表达式结果
  Result lhs = left->print();
  Result rhs = right->print();
  // 若左右表达式结果均为常量，则直接返回常量结果
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
  // 若左右表达式结果不均为常量，则使用临时变量计算结果
  else {
    Result result = Result(Result::Type::REG, fcm.get_temp_count());
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
  // 先计算表达式结果
  Result unary_exp_result = unary_exp->print();
  // 若表达式结果为常量，则直接返回常量结果
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
  // 若表达式结果为临时变量，则使用临时变量计算结果
  else {
    Result result = NEW_REG_(fcm);
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