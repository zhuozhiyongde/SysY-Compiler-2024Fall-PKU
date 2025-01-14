/* 
Bison 语法规则文件，这些规则描述了标记如何组合成语言的语法结构。
定义完成后，工作流程如下：
1. 生成语法分析器：Bison 读取 `.y` 文件并生成一个 C 语言的语法分析器，通常名为 `y.tab.c` 和 `y.tab.h`。
2. 编译语法分析器：使用 C 编译器将 `y.tab.c` 编译成可执行文件。
*/

/* 控制 Bison 的某些行为 */
%code requires {
  /* 编译时包含的头文件 */
  #include <memory>
  #include <string>
  #include "include/ast.hpp"
}

%{
/* 运行时包含的头文件 */
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "include/ast.hpp"

using namespace std;

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数，有的时候是 BaseAST 指针，有的时候是 vector<unique_ptr<BaseAST>> 指针
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  string *str_val;
  int int_val;
  BaseAST *ast_val;
  vector<unique_ptr<BaseAST>> *vec_val;
}

/* token 的声明 */
// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID CONST RETURN
%token IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <str_val> EqOp RelOp AddOp NotOp MulOp AndOp OrOp
%token <int_val> INT_CONST

/* 非终结符的类型定义，ast_val 类型的 */
%type <ast_val> Program CompUnit
%type <ast_val> FuncDef FuncFParam
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal VarDecl VarDef InitVal
%type <ast_val> Block BlockItem Stmt MatchedStmt OpenStmt
%type <ast_val> ConstExp Exp LVal PrimaryExp UnaryExp MulExp AddExp RelExp EqExp LOrExp LAndExp

/* 非终结符的类型定义，vec_val 类型的 */
%type <vec_val> ExtendCompUnit ExtendBlockItem ExtendConstDef ExtendVarDef ExtendFuncFParams ExtendFuncRParams ExtendArrayIndex ExtendConstInitVal ExtendInitVal

/* 非终结符的类型定义，int_val 类型的 */
%type <int_val> Number


%%

/* 规约（即推导的逆步骤，自底向上）到各个非终结符时的语法操作 */

Program
  : CompUnit ExtendCompUnit {
    // make_unique 是 C++14 引入的智能指针模板函数
    // 它会在堆上分配一个 ProgramAST 对象, 并返回一个 unique_ptr<ProgramAST>
    auto program = make_unique<ProgramAST>();
    // CompUnit 的解析返回值
    auto comp_unit = $1;
    // ExtendCompUnit 的解析返回值
    // 这里是传递了指针，而不是值，真实的 vec 当前还存在于 ExtendBlockItemAST 中，通过使用指针，我们可以先避免大量拷贝
    // 也符合了之前对于 ExtendCompUnit 的返回值 yylval 定义 vector<unique_ptr<BaseAST>> *
    vector<unique_ptr<BaseAST>> *vec = $2;
    // 把 $1 即 CompUnit 的解析返回值移动到 program->comp_units 中
    program->comp_units.push_back(unique_ptr<BaseAST>(comp_unit));
    // 把 $2 即 ExtendCompUnit 的解析返回值移动到 program->comp_units 中
    for (auto& ptr : *vec) {
      program->comp_units.push_back(move(ptr));
    }
    // 把 program 移动到 ast 中
    ast = move(program);
  }
  ;

ExtendCompUnit
  : {
    // 如果是从 ε 规约到 ExtendCompUnit, 则创建一个空的 vector<unique_ptr<BaseAST>>
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    // $$ 是 Bison 提供的宏, 它代表当前规则的返回值
    $$ = vec;
  }
  | ExtendCompUnit CompUnit {
    // 如果是从 ExtendCompUnit 规约到 CompUnit, 则把 CompUnit 的解析返回值移动到 ExtendCompUnit 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

CompUnit
  : FuncDef {
    // 是的，可以直接返回下级 AST 的结果，因为我们构建的 AST 推导树不需要此级信息来生成中间代码
    $$ = $1;
  }
  | Decl {
    $$ = $1;
  }
  ;

FuncDef
  : INT IDENT '(' ExtendFuncFParams ')' Block {
    // 有返回值的函数定义
    // 创建一个 FuncDefAST 对象
    auto ast = new FuncDefAST();
    // 设置各种属性
    ast->func_type = FuncDefAST::FuncType::INT;
    ast->ident = *unique_ptr<string>($2);
    vector<unique_ptr<BaseAST>> *vec = $4;
    ast->func_f_params = vec;
    ast->block = unique_ptr<BaseAST>($6);
    // 把 ast 作为当前规则的返回值
    $$ = ast;
  }
  | VOID IDENT '(' ExtendFuncFParams ')' Block {
    // 无返回值的函数定义
    // 创建一个 FuncDefAST 对象
    auto ast = new FuncDefAST();
    // 设置各种属性
    ast->func_type = FuncDefAST::FuncType::VOID;
    ast->ident = *unique_ptr<string>($2);
    vector<unique_ptr<BaseAST>> *vec = $4;
    ast->func_f_params = vec;
    ast->block = unique_ptr<BaseAST>($6);
    // 把 ast 作为当前规则的返回值
    $$ = ast;
  }
  ;

ExtendFuncFParams
  : {
    // 0 个参数
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | FuncFParam {
    // 1 个参数
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ExtendFuncFParams ',' FuncFParam {
    // 多个参数
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

FuncFParam
  : INT IDENT {
    // 普通参数
    auto ast = new FuncFParamAST();
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | INT IDENT '[' ']' ExtendArrayIndex {
    // 数组指针参数
    auto ast = new FuncFParamAST();
    ast->ident = *unique_ptr<string>($2);
    ast->is_array = true;
    vector<unique_ptr<BaseAST>> *vec = $5;
    ast->array_index = vec;
    $$ = ast;
  }
  ;

Block
  : '{' BlockItem ExtendBlockItem '}' {
    // 带语句的块
    auto ast = new BlockAST();
    auto block_item = $2;
    vector<unique_ptr<BaseAST>> *vec = $3;
    ast->block_items.push_back(unique_ptr<BaseAST>(block_item));
    for (auto& ptr : *vec) {
      ast->block_items.push_back(move(ptr));
    }
    $$ = ast;
  }
  | '{' '}'{
    // 空块
    auto ast = new BlockAST();
    ast->block_items = vector<unique_ptr<BaseAST>>();
    $$ = ast;
  }
  ;

ExtendBlockItem
  : {
    // 从 ε 规约到 ExtendBlockItem, 则创建一个空的 vector<unique_ptr<BaseAST>>
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendBlockItem BlockItem {
    // 从 ExtendBlockItem 规约到 BlockItem, 则把 BlockItem 的解析返回值移动到 ExtendBlockItem 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($2));
    $$ = vec;
  }
  ;

BlockItem
  : Decl {
    // 声明语句
    $$ = $1;
  }
  | Stmt {
    // 语句
    $$ = $1;
  }
  ;

Decl
  : ConstDecl {
    // 常量声明
    $$ = $1;
  }
  | VarDecl {
    // 变量声明
    $$ = $1;
  }
  ;

ConstDecl
  : CONST INT ConstDef ExtendConstDef ';' {
    // 常量声明，要处理一行有多个常量定义的情况，如 int a = 1, b = 2;
    auto ast = new ConstDeclAST();
    auto const_def = $3;
    vector<unique_ptr<BaseAST>> *vec = $4;
    ast->const_defs.push_back(unique_ptr<BaseAST>(const_def));
    for (auto& ptr : *vec) {
      ast->const_defs.push_back(move(ptr));
    }
    $$ = ast;
  }
  ;

ExtendConstDef
  : {
    // 从 ε 规约到 ExtendConstDef, 则创建一个空的 vector<unique_ptr<BaseAST>>
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendConstDef ',' ConstDef {
    // 从 ExtendConstDef 规约到 ConstDef, 则把 ConstDef 的解析返回值移动到 ExtendConstDef 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstDef
  : IDENT ExtendArrayIndex '=' ConstInitVal {
    // 常量定义，如 int a[10] = {1, 2, 3}; 或 int a = 1;
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    // 指针肯定有，但是 vector 是否为空需要后续前端处理时判断
    vector<unique_ptr<BaseAST>> *vec = $2;
    ast->array_index = vec;
    ast->value = unique_ptr<BaseAST>($4);
    $$ = ast;
  }

ExtendConstInitVal
  : {
    // 从 ε 规约到 ExtendConstInitVal, 则创建一个空的 vector<unique_ptr<BaseAST>>
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendConstInitVal ',' ConstInitVal {
    // 从 ExtendConstInitVal 规约到 ConstInitVal, 则把 ConstInitVal 的解析返回值移动到 ExtendConstInitVal 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstInitVal
  : ConstExp {
    // 常量初始值，如 const int a = 1 的 1
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    // 常量数组空初始值，如 const int arr = {}; 的 {}
    auto ast = new ConstInitValAST();
    ast->init_values.emplace();
    $$ = ast;
  }
  | '{' ConstInitVal ExtendConstInitVal '}' {
    // 常量数组初始值，如 const int arr = {1, 2, 3}; 的 {1, 2, 3}
    // 注意这个可能会发生嵌套初始化列表定义，如 const int arr = {{1, 2}, {3, 4}};
    auto ast = new ConstInitValAST();
    auto const_init_val = $2;
    vector<unique_ptr<BaseAST>> *vec = $3;
    ast->init_values.emplace();
    ast->init_values->push_back(unique_ptr<BaseAST>(const_init_val));
    for (auto& ptr : *vec) {
      ast->init_values->push_back(move(ptr));
    }
    $$ = ast;
  }
  ;

ExtendArrayIndex
  : {
    // 从 ε 规约到 ExtendArrayIndex, 则创建一个空的 vector<unique_ptr<BaseAST>>
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendArrayIndex '[' ConstExp ']' {
    // 从 ExtendArrayIndex 规约到 ConstExp, 则把 ConstExp 的解析返回值移动到 ExtendArrayIndex 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

ConstExp
  : Exp {
    // 常量表达式，如 const int a = 1 + 2; 的 1 + 2
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : INT VarDef ExtendVarDef ';' {
    // 变量声明，要处理一行有多个变量定义的情况，如 int a, b;
    auto ast = new VarDeclAST();
    auto var_def = $2;
    vector<unique_ptr<BaseAST>> *vec = $3;
    ast->var_defs.push_back(unique_ptr<BaseAST>(var_def));
    for (auto& ptr : *vec) {
      ast->var_defs.push_back(move(ptr));
    }
    $$ = ast;
  }
  ;

ExtendVarDef
  : {
    // 从 ε 规约到 ExtendVarDef, 则创建一个空的 vector<unique_ptr<BaseAST>>
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendVarDef ',' VarDef {
    // 从 ExtendVarDef 规约到 VarDef, 则把 VarDef 的解析返回值移动到 ExtendVarDef 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

VarDef
  : IDENT ExtendArrayIndex {
    // 变量定义，如 int a[10]; 或 int a;
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST>> *vec = $2;
    ast->array_index = vec;
    $$ = ast;
  }
  | IDENT ExtendArrayIndex '=' InitVal {
    // 带初始值的变量定义，如 int a[10] = {1, 2, 3}; 或 int a = 1;
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST>> *vec = $2;
    ast->array_index = vec;
    ast->value = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ExtendInitVal
  : {
    // 从 ε 规约到 ExtendInitVal, 则创建一个空的 vector<unique_ptr<BaseAST>>
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | ExtendInitVal ',' InitVal {
    // 从 ExtendInitVal 规约到 InitVal, 则把 InitVal 的解析返回值移动到 ExtendInitVal 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

InitVal
  : Exp {
    // 初始值，如 int a = 1; 的 1
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    // 空初始值，如 int arr = {}; 的 {}
    auto ast = new InitValAST();
    ast->init_values.emplace();
    $$ = ast;
  }
  | '{' InitVal ExtendInitVal '}' {
    // 数组初始值，如 int arr = {1, 2, 3}; 的 {1, 2, 3}
    // 注意这个可能会发生嵌套初始化列表定义，如 int arr = {{1, 2}, {3, 4}};
    auto ast = new InitValAST();
    auto init_val = $2;
    vector<unique_ptr<BaseAST>> *vec = $3;
    ast->init_values.emplace();
    ast->init_values->push_back(unique_ptr<BaseAST>(init_val));
    for (auto& ptr : *vec) {
      ast->init_values->push_back(move(ptr));
    }
    $$ = ast;
  }
  ;

Stmt
  : MatchedStmt {
    // if-else 完全匹配的语句，保证 else 永远匹配到最近的 if
    $$ = $1;
  }
  | OpenStmt {
    // if-else 不匹配的语句
    $$ = $1;
  }
  ;

MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    // if-else 语句，保证 else 永远匹配到最近的 if
    // 注意，这可能会嵌套定义。
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    // while 语句
    auto ast = new StmtWhileAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    // break 语句
    auto ast = new StmtBreakAST();
    $$ = ast;
  }
  | CONTINUE ';' {
    // continue 语句
    auto ast = new StmtContinueAST();
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    // 赋值语句，如 a = 1;
    auto ast = new StmtAssignAST();
    ast->l_val = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';'{
    // 表达式语句，如 a + 1;
    auto ast = new StmtExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';'{
    // 空语句，如 ;
    auto ast = new StmtExpAST();
    $$ = ast;
  }
  | Block {
    // 块语句，如 { int a = 1; }
    $$ = $1;
  }
  | RETURN Exp ';' {
    // 带返回值的 return 语句，如 return 1;
    auto ast = new StmtReturnAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    // 不带返回值的 return 语句，如 return;
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
    // 保证 else 永远匹配到最近的 if
    auto ast = new StmtIfAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  ;

LVal
  : IDENT ExtendArrayIndex {
    // 左值，如 a[10] 或 a
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST>> *vec = $2;
    ast->array_index = vec;
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    // 或表达式，如 a || b
    auto ast = new ExpAST();
    ast->l_or_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    // 与表达式，如 a && b
    auto ast = new LOrExpAST();
    ast->l_and_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp OrOp LAndExp {
    // 实际上是做了一个优先级处理，AndOp 的优先级高于 OrOp，因为 AndOp 更早规约出来
    auto ast = new LExpWithOpAST();
    ast->logical_op = LExpWithOpAST::LogicalOp::LOGICAL_OR;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    // 等于表达式，如 a == b
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp AndOp EqExp {
    // 实际上是做了一个优先级处理，EqExp 的优先级高于 AndOp，因为 EqExp 更早规约出来
    auto ast = new LExpWithOpAST();
    ast->logical_op = LExpWithOpAST::LogicalOp::LOGICAL_AND;
    ast->left = unique_ptr<BaseAST>($1);
    ast->right = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    // 关系表达式，如 a == b
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EqOp RelExp {
    // 实际上是做了一个优先级处理，RelExp 的优先级高于 EqOp，因为 RelExp 更早规约出来
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
    // 加法表达式，如 a + b
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RelOp AddExp {
    // 实际上是做了一个优先级处理，AddExp 的优先级高于 RelOp，因为 AddExp 更早规约出来
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
    // 乘法表达式，如 a * b
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    // 实际上是做了一个优先级处理，MulExp 的优先级高于 AddOp，因为 MulExp 更早规约出来
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
    // 单目运算符表达式，如 !a 或 -a
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    // 实际上是做了一个优先级处理，UnaryExp 的优先级高于 MulOp，因为 UnaryExp 更早规约出来
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
    // 括号运算符表达式，如 (a)
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddOp UnaryExp {
    // 实际上是做了一个优先级处理，（）的优先级最高
    auto ast = new UnaryExpWithOpAST();
    auto add_op = *unique_ptr<string>($1);
    ast->unary_op = ast->convert(add_op);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | NotOp UnaryExp {
    // 实际上是做了一个优先级处理，（）的优先级最高
    auto ast = new UnaryExpWithOpAST();
    auto not_op = *unique_ptr<string>($1);
    ast->unary_op = ast->convert(not_op);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT '(' ExtendFuncRParams ')' {
    // 函数调用表达式，如 f(1, 2, 3)
    auto ast = new UnaryExpWithFuncCallAST();
    ast->ident = *unique_ptr<string>($1);
    vector<unique_ptr<BaseAST>> *vec = $3;
    ast->func_r_params = vec;
    $$ = ast;
  }
  ;

ExtendFuncRParams
  : {
    // 0 个参数的情况，从 ε 规约到 ExtendFuncRParams, 则创建一个空的 vector<unique_ptr<BaseAST>>
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    $$ = vec;
  }
  | Exp {
    // 1 个参数的情况，从 Exp 规约到 ExtendFuncRParams, 则把 Exp 的解析返回值移动到 ExtendFuncRParams 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = new vector<unique_ptr<BaseAST>>;
    vec->push_back(unique_ptr<BaseAST>($1));
    $$ = vec;
  }
  | ExtendFuncRParams ',' Exp {
    // 多个参数的情况，从 ExtendFuncRParams 规约到 Exp, 则把 Exp 的解析返回值移动到 ExtendFuncRParams 的 vector 中
    vector<unique_ptr<BaseAST>> *vec = $1;
    vec->push_back(unique_ptr<BaseAST>($3));
    $$ = vec;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    // 括号表达式，如 (a)
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    // 数字表达式，如 1
    auto ast = new PrimaryExpWithNumberAST();
    ast->number = $1;
    $$ = ast;
  }
  | LVal {
    // 变量表达式，如 a
    auto ast = new PrimaryExpWithLValAST();
    ast->l_val = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    // 数字表达式，如 1
    $$ = $1;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
