%code requires {
  #include <memory>
  #include <string>
  #include "include/ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
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
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN LE GE EQ NEQ LOGICAL_OR LOGICAL_AND
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt Exp UnaryExp PrimaryExp MulExp AddExp LOrExp LAndExp RelExp EqExp
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
  : '{' Stmt '}' {
    auto ast = new BlockAST();
    ast->stmt = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
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
    $$ = $1;
  }
  | LOrExp LOGICAL_OR LAndExp {
    auto ast = new LExpWithOpAST();
    ast->logical_op = LExpWithOpAST::LogicalOp::LOGICAL_OR;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    $$ = $1;
  }
  | LAndExp LOGICAL_AND EqExp {
    auto ast = new LExpWithOpAST();
    ast->logical_op = LExpWithOpAST::LogicalOp::LOGICAL_AND;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    $$ = $1;
  }
  | EqExp EQ RelExp {
    auto ast = new EqExpWithOpAST();
    ast->eq_op = EqExpWithOpAST::EqOp::EQ;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | EqExp NEQ RelExp {
    auto ast = new EqExpWithOpAST();
    ast->eq_op = EqExpWithOpAST::EqOp::NEQ;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    $$ = $1;
  }
  | RelExp LE AddExp {
    auto ast = new RelExpWithOpAST();
    ast->rel_op = RelExpWithOpAST::RelOp::LE;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GE AddExp {
    auto ast = new RelExpWithOpAST();
    ast->rel_op = RelExpWithOpAST::RelOp::GE;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpWithOpAST();
    ast->rel_op = RelExpWithOpAST::RelOp::LT;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpWithOpAST();
    ast->rel_op = RelExpWithOpAST::RelOp::GT;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    $$ = $1;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpWithOpAST();
    ast->add_op = AddExpWithOpAST::AddOp::ADD;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpWithOpAST();
    ast->add_op = AddExpWithOpAST::AddOp::SUB;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    $$ = $1;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpWithOpAST();
    ast->mul_op = MulExpWithOpAST::MulOp::MUL;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpWithOpAST();
    ast->mul_op = MulExpWithOpAST::MulOp::DIV;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpWithOpAST();
    ast->mul_op = MulExpWithOpAST::MulOp::MOD;
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
  | '+' UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    ast->unary_op = UnaryExpWithOpAST::UnaryOp::POSITIVE;
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | '-' UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    ast->unary_op = UnaryExpWithOpAST::UnaryOp::NEGATIVE;
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | '!' UnaryExp {
    auto ast = new UnaryExpWithOpAST();
    ast->unary_op = UnaryExpWithOpAST::UnaryOp::NOT;
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
    auto ast = new PrimaryExpAST();
    ast->number = $1;
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
