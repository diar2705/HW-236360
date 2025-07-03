#ifndef ANALYZER_HPP
#define ANALYZER_HPP

#include "visitor.hpp"
#include "symbolTable.hpp"
#include "output.hpp"
#include <iostream>

class Analyzer : public Visitor
{
private:
    SymbolTable symbolTable;
    output::CodeBuffer codeBuffer;
    bool inFirstFunction;
    BuiltInType currentReturnType;

public:
    Analyzer();
    void printOutput();
    void setInFirstFunction(bool val);
    bool getInFirstFunction() const;
    BuiltInType getCurrentReturnType() const;
    void setCurrentReturnType(BuiltInType type);
    const output::CodeBuffer &getCodeBuffer() const
    {
        return codeBuffer;
    }

    // Visitor methods
    void visit(ast::Num &node) override;
    void visit(ast::NumB &node) override;
    void visit(ast::String &node) override;
    void visit(ast::Bool &node) override;
    void visit(ast::ID &node) override;
    void visit(ast::BinOp &node) override;
    void visit(ast::RelOp &node) override;
    void visit(ast::Not &node) override;
    void visit(ast::And &node) override;
    void visit(ast::Or &node) override;
    void visit(ast::PrimitiveType &node) override;
    void visit(ast::ArrayType &node) override;
    void visit(ast::ArrayDereference &node) override;
    void visit(ast::ArrayAssign &node) override;
    void visit(ast::Cast &node) override;
    void visit(ast::ExpList &node) override;
    void visit(ast::Call &node) override;
    void visit(ast::Statements &node) override;
    void visit(ast::Break &node) override;
    void visit(ast::Continue &node) override;
    void visit(ast::Return &node) override;
    void visit(ast::If &node) override;
    void visit(ast::While &node) override;
    void visit(ast::VarDecl &node) override;
    void visit(ast::Assign &node) override;
    void visit(ast::Formal &node) override;
    void visit(ast::Formals &node) override;
    void visit(ast::FuncDecl &node) override;
    void visit(ast::Funcs &node) override;
};

#endif // ANALYZER_HPP