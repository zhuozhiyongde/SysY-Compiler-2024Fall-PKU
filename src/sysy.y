%code requires {
  #include <memory>
  #include <string>
  #include "include/ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "include/ast.hpp"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  string *str_val;
  int int_val;
  BaseAST *ast_val;
  vector<unique_ptr<BaseAST>> *vec_val;
}


// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT CONST RETURN IF ELSE
%token <str_val> IDENT
%token <str_val> EqOp RelOp AddOp NotOp MulOp AndOp OrOp
%token <int_val> INT_CONST

// 非终结符的类型定义
// 按照给定语法规范排序
%type <ast_val> CompUnit Decl ConstDecl ConstDef ConstInitVal VarDecl VarDef InitVal
%type <ast_val> FuncDef FuncType
%type <ast_val> Block BlockItem Stmt MatchedStmt OpenStmt
%type <ast_val> ConstExp Exp LVal PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LOrExp LAndExp

%type <vec_val> ExtendBlockItem ExtendConstDef ExtendVarDef

%type <int_val> Number


%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数

CompUnit
  : FuncDef {
    // 创建一个 CompUnitAST 对象
    auto comp_unit = make_unique<CompUnitAST>();
    // $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    // move 是 C++11 引入的移动语义, 用于将 unique_ptr 的所有权从一个变量移动到另一个变量
    // 这里我们把 comp_unit 的所有权移动给 ast, 这样 ast 就指向了 comp_unit 所指向的内存
    ast = move(comp_unit);
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    // $$ 是 Bison 提供的宏, 它代表当前规则的返回值
    $$ = ast;
  }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->type = "i32";
    $$ = ast;
  }
  ;

Block
  : '{' BlockItem ExtendBlockItem '}' {
    auto ast = new BlockAST();
    auto block_item = $2;
    // 这里是传递了指针，而不是值，真实的 vec 当前还存在于 ExtendBlockItemAST 中，通过使用指针，我们可以先避免大量拷贝
    vector<unique_ptr<BaseAST>> *vec = $3;
    ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
    for (auto& ptr : *vec) {
      // 现在把 vec 的值移动到当前的 BlockAST 中
      ast->block_items.push_back(std::move(ptr));
    }
    $$ = ast;
  }
  | '{' '}'{
    auto ast = new BlockAST();
    ast->block_items = vector<unique_ptr<BaseAST>>();
    $$ = ast;
  }
  ;

ExtendBlockItem
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendBlockItem BlockItem {
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

BlockItem
  : Decl {
    $$ = $1;
  }
  | Stmt {
    $$ = $1;
  }
  ;

Decl
  : ConstDecl {
    $$ = $1;
  }
  | VarDecl {
    $$ = $1;
  }
  ;

ConstDecl
  : CONST INT ConstDef ExtendConstDef ';' {
    auto ast = new ConstDeclAST();
    auto const_def = $3;
    vector<unique_ptr<BaseAST>> *vec = $4;
    ast->const_defs.push_back(unique_ptr<BaseAST>(const_def));
    for (auto& ptr : *vec) {
      ast->const_defs.push_back(std::move(ptr));
    }
    $$ = ast;
  }
  ;

ExtendConstDef
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendConstDef ',' ConstDef {
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->value = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : INT VarDef ExtendVarDef ';' {
    auto ast = new VarDeclAST();
    auto var_def = $2;
    vector<unique_ptr<BaseAST>> *vec = $3;
    ast->var_defs.push_back(unique_ptr<BaseAST>(var_def));
    for (auto& ptr : *vec) {
      ast->var_defs.push_back(std::move(ptr));
    }
    $$ = ast;
  }
  ;

ExtendVarDef
  : {
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendVarDef ',' VarDef {
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->value = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt
  : MatchedStmt {
    $$ = $1;
  }
  | OpenStmt {
    $$ = $1;
  }
  ;

MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAssignAST();
    ast->l_val = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';'{
    auto ast = new StmtExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';'{
    auto ast = new StmtExpAST();
    $$ = ast;
  }
  | Block {
    auto ast = new StmtBlockAST();
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtReturnAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtReturnAST();
    $$ = ast;
  }
  ;

OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->l_or_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->l_and_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp OrOp LAndExp {
    auto ast = new LExpWithOpAST();
    ast->logical_op = LExpWithOpAST::LogicalOp::LOGICAL_OR;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp AndOp EqExp {
    auto ast = new LExpWithOpAST();
    ast->logical_op = LExpWithOpAST::LogicalOp::LOGICAL_AND;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EqOp RelExp {
    auto ast = new EqExpWithOpAST();
    auto eq_op = *unique_ptr<string>($2);
    ast->eq_op = ast->convert(eq_op);
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RelOp AddExp {
    auto ast = new RelExpWithOpAST();
    auto rel_op = *unique_ptr<string>($2);
    ast->rel_op = ast->convert(rel_op);
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpWithOpAST();
    auto add_op = *unique_ptr<string>($2);
    ast->add_op = ast->convert(add_op);
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpWithOpAST();
    auto mul_op = *unique_ptr<string>($2);
    ast->mul_op = ast->convert(mul_op);
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddOp UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    auto add_op = *unique_ptr<string>($1);
    ast->unary_op = ast->convert(add_op);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | NotOp UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    auto not_op = *unique_ptr<string>($1);
    ast->unary_op = ast->convert(not_op);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpWithNumberAST();
    ast->number = $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpWithLValAST();
    ast->l_val = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
