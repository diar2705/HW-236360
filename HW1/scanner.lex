%{
#include <stdio.h>
#include "output.hpp"
void concat(const char* str);
void escapeCheck(const char* str);
%}

%option yylineno
%option noyywrap
whitespace ([ \t\n])
digit ([0-9])
letter ([a-zA-Z])
id ({letter}({letter}|{digit})*)
num (0|[1-9]{digit}*)
quote ([\"])
char  ([^\"\\[:cntrl:]])
escapeSeq (\\[\\|n|r|t|0|"]|\\x[2-6][0-9a-fA-F]|\\x7[0-9a-eA-E])
tab (\t)

%x STR 
%%
(void)      output::printToken(yylineno, tokentype::VOID, yytext);
(int)       output::printToken(yylineno, tokentype::INT, yytext);
(byte)      output::printToken(yylineno, tokentype::BYTE, yytext);
(bool)      output::printToken(yylineno, tokentype::BOOL, yytext);
(and)       output::printToken(yylineno, tokentype::AND, yytext);
(or)        output::printToken(yylineno, tokentype::OR, yytext);
(not)       output::printToken(yylineno, tokentype::NOT, yytext);
(true)      output::printToken(yylineno, tokentype::TRUE, yytext);
(false)     output::printToken(yylineno, tokentype::FALSE, yytext);
(return)    output::printToken(yylineno, tokentype::RETURN, yytext);
(if)        output::printToken(yylineno, tokentype::IF, yytext);
(else)      output::printToken(yylineno, tokentype::ELSE, yytext);
(while)     output::printToken(yylineno, tokentype::WHILE, yytext);
(break)     output::printToken(yylineno, tokentype::BREAK, yytext);
(continue)  output::printToken(yylineno, tokentype::CONTINUE, yytext);
(;)        output::printToken(yylineno, tokentype::SC, yytext);
(,)         output::printToken(yylineno, tokentype::COMMA, yytext);
(\()         output::printToken(yylineno, tokentype::LPAREN, yytext);
(\))         output::printToken(yylineno, tokentype::RPAREN, yytext);
(\{)         output::printToken(yylineno, tokentype::LBRACE, yytext);
(\})         output::printToken(yylineno, tokentype::RBRACE, yytext);
(\[)         output::printToken(yylineno, tokentype::LBRACK, yytext);
(\])         output::printToken(yylineno, tokentype::RBRACK, yytext);
(=)         output::printToken(yylineno, tokentype::ASSIGN, yytext);
(==|<|>|<=|>=|!=) output::printToken(yylineno, tokentype::RELOP, yytext);
(\+|-|\*|\/)     output::printToken(yylineno, tokentype::BINOP, yytext);
(\/\/.*)    output::printToken(yylineno, tokentype::COMMENT, yytext);
{id}        output::printToken(yylineno, tokentype::ID, yytext);
{num}       output::printToken(yylineno, tokentype::NUM, yytext);
{quote}  BEGIN(STR);
<STR>{char}*    concat(yytext);
<STR>{escapeSeq}    escapeCheck(yytext);
<STR>{tab}*    concat(yytext); 
<STR>\\x[^\n\r\t\" ]{1,2}    output::errorUndefinedEscape(yytext + 1);
<STR>\\.    output::errorUndefinedEscape(yytext + 1);
<STR><<EOF>>    output::errorUnclosedString();
<STR>{quote}    BEGIN(INITIAL); return tokentype::STRING;
{whitespace}    ;/* ignore whitespace */
.           output::errorUnknownChar(*yytext);

%%
