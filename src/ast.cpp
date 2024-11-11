#include "include/ast.hpp"

void CompUnitAST::print() const {
  printf("CompUnitAST { ");
  func_def->print();
  printf(" }");
}

void FuncDefAST::print() const {
  printf("FuncDefAST { ");
  func_type->print();
  printf(", %s, ", ident.c_str());
  block->print();
  printf(" }");
}

void FuncTypeAST::print() const {
  printf("FuncTypeAST { %s }", type.c_str());
}

void BlockAST::print() const {
  printf("BlockAST { ");
  stmt->print();
  printf(" }");
}

void StmtAST::print() const {
  printf("StmtAST { %d }", number);
}

