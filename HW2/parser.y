%{

#include "nodes.hpp"
#include "output.hpp"

// bison declarations
extern int yylineno;
extern int yylex();

void yyerror(const char*);

// root of the AST, set by the parser and used by other parts of the compiler
std::shared_ptr<ast::Node> program;
//std::shared_ptr<ast::Node> node_ptr;

using namespace std;
// Commented out because we're using %union instead
// #define YYSTYPE std::shared_ptr<ast::Node>
extern char* yytext;
%}

// Define the union for semantic values


%token VOID
%token INT
%token BYTE
%token BOOL
%token TRUE
%token FALSE
%token RETURN
%token IF
%token WHILE
%token BREAK
%token CONTINUE
%token SC
%token COMMA
%token RPAREN
%token LBRACE
%token RBRACE
%token LBRACK
%token RBRACK
%token ID
%token NUM
%token NUM_B
%token STRING

%left OR
%left AND
%right ASSIGN
%left RELOP_LOW
%left RELOP_HIGH
%left BINOP_LOW
%left BINOP_HIGH
%right NOT

%nonassoc IFX
%nonassoc ELSE
%nonassoc   ID_AS_VALUE     
%left       LPAREN

%precedence CASTING

// Define the types for non-terminals
//%type <node_ptr> Program Funcs FuncDecl RetType Formals FormalsList FormalDecl Statements Statement Call ExpList Type Exp

%%

// While reducing the start variable, set the root of the AST
Program:  Funcs { program = $1; }
;

Funcs:                  { $$ = std::make_shared<ast::Funcs>(); }
|   FuncDecl Funcs      { 
                            auto funcs = std::dynamic_pointer_cast<ast::Funcs>($2);
                            funcs->push_front(std::dynamic_pointer_cast<ast::FuncDecl>($1));
                            $$ = funcs;
                        }
;

FuncDecl:  RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
        { 
            $$ = std::make_shared<ast::FuncDecl>(
                std::dynamic_pointer_cast<ast::ID>($2),
                std::dynamic_pointer_cast<ast::Type>($1),
                std::dynamic_pointer_cast<ast::Formals>($4),
                std::dynamic_pointer_cast<ast::Statements>($7)
            );
        }
;

RetType:  Type { $$ = $1; }
|   VOID { $$ = std::make_shared<ast::PrimitiveType>(ast::BuiltInType::VOID); }
;

Formals:  { $$ = std::make_shared<ast::Formals>(); }
|   FormalsList { $$ = $1; }
;

FormalsList:  FormalDecl { $$ = std::make_shared<ast::Formals>(std::dynamic_pointer_cast<ast::Formal>($1));}
|   FormalDecl COMMA FormalsList { 
            auto formalsList = std::dynamic_pointer_cast<ast::Formals>($3);
            formalsList->push_front(std::dynamic_pointer_cast<ast::Formal>($1));
            $$ = formalsList;
        }
;

FormalDecl:  Type ID { 
                $$ = std::make_shared<ast::Formal>(
                    std::dynamic_pointer_cast<ast::ID>($2),
                    std::dynamic_pointer_cast<ast::Type>($1)
                );
            }
;

Statements:  Statement {$$ = std::make_shared<ast::Statements>(std::dynamic_pointer_cast<ast::Statement>($1));}
|   Statements Statement { 
                            auto statements = std::dynamic_pointer_cast<ast::Statements>($1);
                            statements->push_back(std::dynamic_pointer_cast<ast::Statement>($2));
                            $$ = statements;
                        }
;

Statement:  LBRACE Statements RBRACE { $$ = $2; }
|   Type ID SC { 
            $$ = std::make_shared<ast::VarDecl>(
                std::dynamic_pointer_cast<ast::ID>($2),
                std::dynamic_pointer_cast<ast::Type>($1)
            );
        }
|   Type ID ASSIGN Exp SC { 
            $$ = std::make_shared<ast::VarDecl>(
                std::dynamic_pointer_cast<ast::ID>($2),
                std::dynamic_pointer_cast<ast::Type>($1),
                std::dynamic_pointer_cast<ast::Exp>($4)
            );
        }
|   ID ASSIGN Exp SC { 
            $$ = std::make_shared<ast::Assign>(
                std::dynamic_pointer_cast<ast::ID>($1),
                std::dynamic_pointer_cast<ast::Exp>($3)
            );
        }
|   ID LBRACK Exp RBRACK ASSIGN Exp SC { 
            $$ = std::make_shared<ast::ArrayAssign>(
                std::dynamic_pointer_cast<ast::ID>($1),
                std::dynamic_pointer_cast<ast::Exp>($6),
                std::dynamic_pointer_cast<ast::Exp>($3)
            );
        }
|   Type ID LBRACK Exp RBRACK SC { 
            auto primitiveType = std::dynamic_pointer_cast<ast::PrimitiveType>($1);
            auto type = std::make_shared<ast::ArrayType>(
                primitiveType->type,
                std::dynamic_pointer_cast<ast::Exp>($4)
            );
            $$ = std::make_shared<ast::VarDecl>(
                std::dynamic_pointer_cast<ast::ID>($2),
                type
            );
        }
|   Call SC { $$ = $1; }
|   RETURN SC { $$ = std::make_shared<ast::Return>(); }
|   RETURN Exp SC { 
            $$ = std::make_shared<ast::Return>(
                std::dynamic_pointer_cast<ast::Exp>($2)
            );
        }
|   IF LPAREN Exp RPAREN Statement %prec IFX { 
            $$ = std::make_shared<ast::If>(
                std::dynamic_pointer_cast<ast::Exp>($3),
                std::dynamic_pointer_cast<ast::Statement>($5)
            );
        }
|   IF LPAREN Exp RPAREN Statement ELSE Statement { 
            $$ = std::make_shared<ast::If>(
                std::dynamic_pointer_cast<ast::Exp>($3),
                std::dynamic_pointer_cast<ast::Statement>($5),
                std::dynamic_pointer_cast<ast::Statement>($7)
            );
        }
|   WHILE LPAREN Exp RPAREN Statement { 
            $$ = std::make_shared<ast::While>(
                std::dynamic_pointer_cast<ast::Exp>($3),
                std::dynamic_pointer_cast<ast::Statement>($5)
            );
        }
|   BREAK SC { $$ = std::make_shared<ast::Break>(); }
|   CONTINUE SC { $$ = std::make_shared<ast::Continue>(); }
;

Call:  ID LPAREN ExpList RPAREN { 
            $$ = std::make_shared<ast::Call>(
                std::dynamic_pointer_cast<ast::ID>($1),
                std::dynamic_pointer_cast<ast::ExpList>($3)
            );
        }
|   ID LPAREN RPAREN { 
            $$ = std::make_shared<ast::Call>(
                std::dynamic_pointer_cast<ast::ID>($1)
            );
        }
;

ExpList:  Exp { $$ = std::make_shared<ast::ExpList>(std::dynamic_pointer_cast<ast::Exp>($1)); }
|   Exp COMMA ExpList { 
            auto expList = std::dynamic_pointer_cast<ast::ExpList>($3);
            expList->push_front(std::dynamic_pointer_cast<ast::Exp>($1));
            $$ = expList;
        }
;

Type:  INT { $$ = std::make_shared<ast::PrimitiveType>(ast::BuiltInType::INT); }
|   BYTE { $$ = std::make_shared<ast::PrimitiveType>(ast::BuiltInType::BYTE); }
|   BOOL { $$ = std::make_shared<ast::PrimitiveType>(ast::BuiltInType::BOOL); }
;

Exp:    LPAREN Exp RPAREN { $$ = $2; }
|   ID LBRACK Exp RBRACK { 
            $$ = std::make_shared<ast::ArrayDereference>(
                std::dynamic_pointer_cast<ast::ID>($1),
                std::dynamic_pointer_cast<ast::Exp>($3)
            );
        }

|   Exp BINOP_LOW Exp {
            $$ = std::make_shared<ast::BinOp>(
                std::dynamic_pointer_cast<ast::Exp>($1),
                std::dynamic_pointer_cast<ast::Exp>($3),
                std::dynamic_pointer_cast<ast::BinOp>($2)->op
            );
        }
|    Exp BINOP_HIGH Exp {
            $$ = std::make_shared<ast::BinOp>(
                std::dynamic_pointer_cast<ast::Exp>($1),
                std::dynamic_pointer_cast<ast::Exp>($3),
                std::dynamic_pointer_cast<ast::BinOp>($2)->op
            );
        }

|   ID  %prec ID_AS_VALUE { $$ = $1; }
|   Call { $$ = $1; }
|   NUM { $$ = $1; }
|   NUM_B { $$ = $1; }
|   STRING { $$ = $1; }
|   TRUE { $$ = std::make_shared<ast::Bool>(true); }
|   FALSE { $$ = std::make_shared<ast::Bool>(false); }
|   NOT Exp { 
            $$ = std::make_shared<ast::Not>(
                std::dynamic_pointer_cast<ast::Exp>($2)
            );
        }
|   Exp AND Exp { 
            $$ = std::make_shared<ast::And>(
                std::dynamic_pointer_cast<ast::Exp>($1),
                std::dynamic_pointer_cast<ast::Exp>($3)
            );
        }
|   Exp OR Exp { 
            $$ = std::make_shared<ast::Or>(
                std::dynamic_pointer_cast<ast::Exp>($1),
                std::dynamic_pointer_cast<ast::Exp>($3)
            );
        }



|   Exp RELOP_LOW Exp { 
            $$ = std::make_shared<ast::RelOp>(
                std::dynamic_pointer_cast<ast::Exp>($1),
                std::dynamic_pointer_cast<ast::Exp>($3),
                std::dynamic_pointer_cast<ast::RelOp>($2)->op
            );
        }
|    Exp RELOP_HIGH Exp { 
            $$ = std::make_shared<ast::RelOp>(
                std::dynamic_pointer_cast<ast::Exp>($1),
                std::dynamic_pointer_cast<ast::Exp>($3),
                std::dynamic_pointer_cast<ast::RelOp>($2)->op
            );
        }

|   LPAREN Type RPAREN Exp %prec CASTING { 
            auto type = std::dynamic_pointer_cast<ast::Type>($2);
            auto primitiveType = std::dynamic_pointer_cast<ast::PrimitiveType>(type);
            $$ = std::make_shared<ast::Cast>(
                std::dynamic_pointer_cast<ast::Exp>($4),
                primitiveType
            );
        }


%%

void yyerror(const char* message) {
    output::errorSyn(yylineno);
    exit(0);
}
