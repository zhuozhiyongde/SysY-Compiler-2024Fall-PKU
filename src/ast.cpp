#include "include/ast.hpp"

void CompUnitAST::print() const {
  if (mode == "-debug") {
    printf("CompUnitAST { ");
    func_def->print();
    printf(" }");
  }
  else if (mode == "-koopa") {
    func_def->print();
  }
}

void FuncDefAST::print() const {
  if (mode == "-debug") {
    printf("FuncDefAST { ");
    func_type->print();
    printf(", %s, ", ident.c_str());
    block->print();
    printf(" }");
  }
  else if (mode == "-koopa") {
    printf("fun @%s(): ", ident.c_str());
    func_type->print();
    printf(" {\n");
    block->print();
    printf("}\n");
  }
}

void FuncTypeAST::print() const {
  if (mode == "-debug") {
    printf("FuncTypeAST { %s }", type.c_str());
  }
  else if (mode == "-koopa") {
    if (type == "int") {
      printf("i32");
    }
    else if (type == "void") {
      printf("void");
    }
  }
}

void BlockAST::print() const {
  if (mode == "-debug") {
    printf("BlockAST { ");
    stmt->print();
    printf(" }");
  }
  else if (mode == "-koopa") {
    printf("%%entry:\n");
    stmt->print();
  }
}

void StmtAST::print() const {
  if (mode == "-debug") {
    printf("StmtAST { %d }", number);
  }
  else if (mode == "-koopa") {
    printf("  ret %d\n", number);
  }
}

