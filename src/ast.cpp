#include "include/ast.hpp"

void CompUnitAST::print() const {
  if (mode == "-debug") {
    cout << "CompUnitAST { ";
    func_def->print();
    cout << " }";
  }
  else if (mode == "-koopa") {
    func_def->print();
  }
}

void FuncDefAST::print() const {
  if (mode == "-debug") {
    cout << "FuncDefAST { ";
    func_type->print();
    cout << ", " << ident << ", ";
    block->print();
    cout << " }";
  }
  else if (mode == "-koopa") {
    cout << "fun @" << ident << "(): ";
    func_type->print();
    cout << " {" << endl;
    block->print();
    cout << "}" << endl;
  }
}

void FuncTypeAST::print() const {
  if (mode == "-debug") {
    cout << "FuncTypeAST { " << type << " }";
  }
  else if (mode == "-koopa") {
    if (type == "int") {
      cout << "i32";
    }
    else if (type == "void") {
      cout << "void";
    }
  }
}

void BlockAST::print() const {
  if (mode == "-debug") {
    cout << "BlockAST { ";
    stmt->print();
    cout << " }";
  }
  else if (mode == "-koopa") {
    cout << "%entry:" << endl;
    stmt->print();
  }
}

void StmtAST::print() const {
  if (mode == "-debug") {
    cout << "StmtAST { " << number << " }";
  }
  else if (mode == "-koopa") {
    cout << "  ret " << number << endl;
  }
}

