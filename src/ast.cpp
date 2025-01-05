#include "include/ast.hpp"

SymbolTable global_symbol_table;
SymbolTable* local_symbol_table = &global_symbol_table;
EnvironmentManager environment_manager;

Result ProgramAST::print() const {
  init_lib();
  for (auto& comp_unit : comp_units) {
    comp_unit->print();
  }
  return Result();
}

Result FuncDefAST::print() const {
  koopa_ofs << endl;
  environment_manager.is_global = false;
  SymbolTable* parent_symbol_table = local_symbol_table;
  local_symbol_table = new SymbolTable();
  local_symbol_table->set_parent(parent_symbol_table);
  koopa_ofs << "fun @" << ident;
  koopa_ofs << "(";
  if (func_f_params) {
    for (size_t i = 0; i < func_f_params->size(); i++) {
      // 转换为 FuncFParamAST
      ((FuncFParamAST*)func_f_params->at(i).get())->as_param();
      if (i != func_f_params->size() - 1) {
        koopa_ofs << ", ";
      }
    }
  }
  koopa_ofs << ")";
  if (func_type == FuncType::INT) {
    koopa_ofs << ": i32";
    environment_manager.is_func_return[ident] = true;
  }
  else {
    environment_manager.is_func_return[ident] = false;
  }
  koopa_ofs << " {" << endl;
  koopa_ofs << "%entry:" << endl;
  for (auto& item : *func_f_params) {
    item->print();
  }
  block->print();
  if (func_type == FuncType::INT) {
    koopa_ofs << "\tret 0" << endl;
  }
  else {
    koopa_ofs << "\tret" << endl;
  }
  koopa_ofs << "}" << endl;
  delete local_symbol_table;
  local_symbol_table = parent_symbol_table;
  environment_manager.is_global = true;
  return Result();
}

void FuncFParamAST::as_param() const {
  koopa_ofs << "@" << ident << ": i32";
}

Result FuncFParamAST::print() const {
  //  %x = alloc i32
  // store @x, %x
  // 在当前层级符号表中创建变量
  string ident_with_suffix = local_symbol_table->assign(ident);
  koopa_ofs << "\t@" << ident_with_suffix << " = alloc i32" << endl;
  koopa_ofs << "\tstore @" << ident << ", @" << ident_with_suffix << endl;
  // 在当前层级符号表中创建变量
  local_symbol_table->create(ident_with_suffix, VAR_);
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
  if (environment_manager.is_global) {
    // global @var = alloc i32, zeroinit
    if (value) {
      Result value_result = (*value)->print();
      assert(value_result.type == Result::Type::IMM);
      koopa_ofs << "global @" << ident_with_suffix << " = alloc i32, " << value_result << endl;
    }
    else {
      koopa_ofs << "global @" << ident_with_suffix << " = alloc i32, zeroinit" << endl;
    }
  }
  else {
    // 判断是否需要生成 alloc 指令
    if (!environment_manager.is_symbol_allocated[ident_with_suffix]) {
      koopa_ofs << "\t@" << ident_with_suffix << " = alloc i32" << endl;
      environment_manager.is_symbol_allocated[ident_with_suffix] = true;
    }
    // 若初始值不为空，则生成 store 指令
    if (value) {
      Result value_result = (*value)->print();
      koopa_ofs << "\tstore " << value_result << ", @" << ident_with_suffix << endl;
    }
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
  string then_label = environment_manager.get_then_label();
  string else_label = environment_manager.get_else_label();
  string end_label = environment_manager.get_end_label();
  environment_manager.add_if_else_count();
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

Result StmtWhileAST::print() const {
  // 准备标签
  string entry_label = environment_manager.get_while_entry_label();
  string body_label = environment_manager.get_while_body_label();
  string end_label = environment_manager.get_while_end_label();
  auto old_while_current = environment_manager.get_while_current();
  environment_manager.set_while_current(environment_manager.get_while_count());
  environment_manager.add_while_count();

  // 生成 while 循环
  koopa_ofs << "\tjump " << entry_label << endl;
  koopa_ofs << entry_label << ":" << endl;
  Result exp_result = exp->print();
  // 创建新的符号表，避免 while 语句中的单句 return 修改当前块 is_returned
  SymbolTable* parent_symbol_table = local_symbol_table;
  local_symbol_table = new SymbolTable();
  local_symbol_table->set_parent(parent_symbol_table);
  koopa_ofs << "\tbr " << exp_result << ", " << body_label << ", " << end_label << endl;
  koopa_ofs << body_label << ":" << endl;
  stmt->print();
  koopa_ofs << "\tjump " << entry_label << endl;
  koopa_ofs << end_label << ":" << endl;
  // 恢复符号表
  delete local_symbol_table;
  local_symbol_table = parent_symbol_table;
  environment_manager.set_while_current(old_while_current);
  return Result();
}

Result StmtBreakAST::print() const {
  koopa_ofs << "\tjump " << environment_manager.get_while_end_label(true) << endl;
  auto jump_label = environment_manager.get_jump_label();
  koopa_ofs << jump_label << ":" << endl;
  return Result();
}

Result StmtContinueAST::print() const {
  koopa_ofs << "\tjump " << environment_manager.get_while_entry_label(true) << endl;
  auto jump_label = environment_manager.get_jump_label();
  koopa_ofs << jump_label << ":" << endl;
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
  auto jump_label = environment_manager.get_jump_label();
  koopa_ofs << jump_label << ":" << endl;
  return Result();
}

Result LValAST::print() const {
  // 获取变量名在符号表中的位置，然后读取其值
  string ident_with_suffix = local_symbol_table->locate(ident);
  assert(local_symbol_table->exist(ident_with_suffix));
  auto symbol = local_symbol_table->read(ident_with_suffix);
  // 若变量是变量，则使用 load 指令读取其值
  if (symbol.type == Symbol::Type::VAR) {
    Result result = NEW_REG_;
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

  if (logical_op == LogicalOp::LOGICAL_OR) {
    // 左侧为立即数
    if (lhs.type == Result::Type::IMM) {
      if (lhs.value != 0) {
        return IMM_(1);
      }
      else {
        Result rhs = right->print();
        return rhs;
      }
    }
    // 左侧不为立即数
    auto true_label = environment_manager.get_short_true_label();
    auto false_label = environment_manager.get_short_false_label();
    auto end_label = environment_manager.get_short_end_label();
    auto result = environment_manager.get_short_result_reg();
    environment_manager.add_short_circuit_count();

    // 判断是否需要生成 alloc 指令
    if (!environment_manager.is_symbol_allocated[result]) {
      koopa_ofs << "\t" << result << " = alloc i32" << endl;
      environment_manager.is_symbol_allocated[result] = true;
    }

    koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;
    koopa_ofs << true_label << ":" << endl;
    koopa_ofs << "\t" << "store 1, " << result << endl;
    koopa_ofs << "\tjump " << end_label << endl;

    koopa_ofs << false_label << ":" << endl;
    Result rhs = right->print();
    Result temp_1 = NEW_REG_;
    Result temp_2 = NEW_REG_;
    koopa_ofs << "\t" << temp_1 << " = or " << rhs << ", 0" << endl;
    koopa_ofs << "\t" << temp_2 << " = ne " << temp_1 << ", 0" << endl;
    koopa_ofs << "\t" << "store " << temp_2 << ", " << result << endl;
    koopa_ofs << "\tjump " << end_label << endl;
    koopa_ofs << end_label << ":" << endl;
    Result result_reg = NEW_REG_;
    koopa_ofs << "\t" << result_reg << " = load " << result << endl;

    return result_reg;
  }
  else if (logical_op == LogicalOp::LOGICAL_AND) {
    // 左侧为立即数
    if (lhs.type == Result::Type::IMM) {
      if (lhs.value == 0) {
        return IMM_(0);
      }
      else {
        Result rhs = right->print();
        return rhs;
      }
    }
    // 左侧不为立即数
    auto true_label = environment_manager.get_short_true_label();
    auto false_label = environment_manager.get_short_false_label();
    auto end_label = environment_manager.get_short_end_label();
    auto result = environment_manager.get_short_result_reg();
    environment_manager.add_short_circuit_count();

    // 判断是否需要生成 alloc 指令
    if (!environment_manager.is_symbol_allocated[result]) {
      koopa_ofs << "\t" << result << " = alloc i32" << endl;
      environment_manager.is_symbol_allocated[result] = true;
    }

    koopa_ofs << "\tbr " << lhs << ", " << true_label << ", " << false_label << endl;
    koopa_ofs << false_label << ":" << endl;
    koopa_ofs << "\t" << "store 0, " << result << endl;
    koopa_ofs << "\tjump " << end_label << endl;

    koopa_ofs << true_label << ":" << endl;
    Result rhs = right->print();
    Result temp_1 = NEW_REG_;
    Result temp_2 = NEW_REG_;
    Result temp_3 = NEW_REG_;
    koopa_ofs << "\t" << temp_1 << " = ne " << lhs << ", 0" << endl;
    koopa_ofs << "\t" << temp_2 << " = ne " << rhs << ", 0" << endl;
    koopa_ofs << "\t" << temp_3 << " = and " << temp_1 << ", " << temp_2 << endl;
    koopa_ofs << "\t" << "store " << temp_3 << ", " << result << endl;
    koopa_ofs << "\tjump " << end_label << endl;
    koopa_ofs << end_label << ":" << endl;
    Result result_reg = NEW_REG_;
    koopa_ofs << "\t" << result_reg << " = load " << result << endl;

    return result_reg;
  }
  else {
    assert(false);
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
    Result result = NEW_REG_;
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
    Result result = NEW_REG_;
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
    Result result = NEW_REG_;
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
    Result result = NEW_REG_;
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
    Result result = NEW_REG_;
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

Result UnaryExpWithFuncCallAST::print() const {
  vector<Result> params;
  for (auto& param : *func_r_params) {
    params.push_back(param->print());
  }

  if (environment_manager.is_func_return[ident]) {
    Result result = NEW_REG_;
    koopa_ofs << "\t" << result << " = call @" << ident << "(";
    for (size_t i = 0; i < params.size(); i++) {
      koopa_ofs << params[i];
      if (i != params.size() - 1) {
        koopa_ofs << ", ";
      }
    }
    koopa_ofs << ")" << endl;
    return result;
  }
  else {
    koopa_ofs << "\tcall @" << ident << "(";
    for (size_t i = 0; i < params.size(); i++) {
      koopa_ofs << params[i];
      if (i != params.size() - 1) {
        koopa_ofs << ", ";
      }
    }
    koopa_ofs << ")" << endl;
    return Result();
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