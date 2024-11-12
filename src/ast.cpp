#include "include/ast.hpp"

void CompUnitAST::print() const {
  if (mode == "-debug") {
    debug_ofs << "CompUnitAST { ";
    func_def->print();
    debug_ofs << " }";
  }
  else if (mode == "-koopa" || mode == "-riscv") {
    func_def->print();
  }
}

void FuncDefAST::print() const {
  if (mode == "-debug") {
    debug_ofs << "FuncDefAST { ";
    func_type->print();
    debug_ofs << ", " << ident << ", ";
    block->print();
    debug_ofs << " }";
  }
  else if (mode == "-koopa" || mode == "-riscv") {
    koopa_ofs << "fun @" << ident << "(): ";
    func_type->print();
    koopa_ofs << " {" << endl;
    block->print();
    koopa_ofs << "}" << endl;
  }
}

void FuncTypeAST::print() const {
  if (mode == "-debug") {
    debug_ofs << "FuncTypeAST { " << type << " }";
  }
  else if (mode == "-koopa" || mode == "-riscv") {
    if (type == "int") {
      koopa_ofs << "i32";
    }
    else if (type == "void") {
      koopa_ofs << "void";
    }
  }
}

void BlockAST::print() const {
  if (mode == "-debug") {
    debug_ofs << "BlockAST { ";
    stmt->print();
    debug_ofs << " }";
  }
  else if (mode == "-koopa" || mode == "-riscv") {
    koopa_ofs << "%entry:" << endl;
    stmt->print();
  }
}

void StmtAST::print() const {
  if (mode == "-debug") {
    debug_ofs << "StmtAST { " << number << " }";
  }
  else if (mode == "-koopa" || mode == "-riscv") {
    koopa_ofs << "  ret " << number << endl;
  }
}

