%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>

// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "sysy.tab.hpp"

using namespace std;

%}

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*
BlockComment  \/\*([^\*]*|[\*]+[^\*\/])*[\*]+\/

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

/* 运算关系符 */
EqOp          ("=="|"!=")
RelOp         ("<"|">"|"<="|">=")
AddOp         ("+"|"-")
NotOp         ("!")
MulOp         ("*"|"/"|"%")
AndOp         "&&"
OrOp          "||"

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }
{BlockComment}  { /* 忽略, 不做任何操作 */ }

"int"           { return INT; }
"const"         { return CONST; }
"return"        { return RETURN; }
"if"            { return IF; }
"else"          { return ELSE; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

{EqOp}          { yylval.str_val = new string(yytext); return EqOp; }
{RelOp}         { yylval.str_val = new string(yytext); return RelOp; }
{AddOp}         { yylval.str_val = new string(yytext); return AddOp; }
{NotOp}         { yylval.str_val = new string(yytext); return NotOp; }
{MulOp}         { yylval.str_val = new string(yytext); return MulOp; }
{AndOp}         { yylval.str_val = new string(yytext); return AndOp; }
{OrOp}          { yylval.str_val = new string(yytext); return OrOp; }
.               { return yytext[0]; }

%%
