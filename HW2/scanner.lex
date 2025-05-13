%{
#include <stdio.h>
#include <string.h>
#include "nodes.hpp"
#include "output.hpp"
#include "parser.tab.h"
%}

%option yylineno
%option noyywrap

whitespace      ([\t\n\r ])
comment         (\/\/[^\r\n]*[ \r|\n|\r\n]?)
id              ([a-zA-Z][a-zA-Z0-9]*)
number          (0|[1-9][0-9]*)
byte            ({number}b)
string          (\"([^\n\r\"\\]|\\[rnt"\\])+\")

%%
"void"            return VOID;
"int"             return INT;
"byte"            return BYTE;
"bool"            return BOOL;
"and"             return AND;
"or"              return OR;
"not"             return NOT;
"true"            return TRUE;
"false"           return FALSE;
"return"          return RETURN;
"if"              return IF;
"else"            return ELSE;
"while"           return WHILE;
"break"           return BREAK;
"continue"        return CONTINUE;
";"             return SC;
","             return COMMA;
"("             return LPAREN;
")"             return RPAREN;
"{"             return LBRACE;
"}"             return RBRACE;
"["             return LBRACK;
"]"             return RBRACK;
"="             return ASSIGN;

"=="            { yylval = std::make_shared<ast::RelOp>(nullptr, nullptr, ast::RelOpType::EQ); return RELOP; }
"!="            { yylval = std::make_shared<ast::RelOp>(nullptr, nullptr, ast::RelOpType::NE); return RELOP; }
"<"             { yylval = std::make_shared<ast::RelOp>(nullptr, nullptr, ast::RelOpType::LT); return RELOP; }
">"             { yylval = std::make_shared<ast::RelOp>(nullptr, nullptr, ast::RelOpType::GT); return RELOP; }
"<="            { yylval = std::make_shared<ast::RelOp>(nullptr, nullptr, ast::RelOpType::LE); return RELOP; }
">="            { yylval = std::make_shared<ast::RelOp>(nullptr, nullptr, ast::RelOpType::GE); return RELOP; }
"+"             { yylval = std::make_shared<ast::BinOp>(nullptr, nullptr, ast::BinOpType::ADD); return BINOP; }
"-"             { yylval = std::make_shared<ast::BinOp>(nullptr, nullptr, ast::BinOpType::SUB); return BINOP; }
"*"             { yylval = std::make_shared<ast::BinOp>(nullptr, nullptr, ast::BinOpType::MUL); return BINOP; }
"/"             { yylval = std::make_shared<ast::BinOp>(nullptr, nullptr, ast::BinOpType::DIV); return BINOP; }

{id}            { yylval = std::make_shared<ast::ID>(yytext); return ID; }
{number}        { yylval = std::make_shared<ast::Num>(yytext); return NUM; }
{byte}          { yylval = std::make_shared<ast::NumB>(yytext); return NUM_B; }
{string}        { yylval = std::make_shared<ast::String>(yytext); return STRING; }

{whitespace}    { /* Ignore whitespace */ }
{comment}       { /* Ignore comments */ }
.               { output::errorLex(yylineno); }

%%