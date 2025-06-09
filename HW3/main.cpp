#include "output.hpp"
#include "nodes.hpp"
#include "analyzer.hpp"

// Extern from the bison-generated parser
extern int yyparse();

extern std::shared_ptr<ast::Node> program;

int main()
{
    // Parse the input. The result is stored in the global variable `program`
    yyparse();

    // Print the AST using the PrintVisitor
    Analyzer analyzer;
    program->accept(analyzer);
    analyzer.printOutput();
    // std::cout << analyzer.getPrinter();
}
