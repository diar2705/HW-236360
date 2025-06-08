#include "analyzer.hpp"

using namespace std;

Analyzer::Analyzer() : symbolTable(), printer(), inFirstFunction(false) {}

void Analyzer::printOutput()
{
    cout << printer;
}

void Analyzer::setInFirstFunction(bool val)
{
    inFirstFunction = val;
}

bool Analyzer::getInFirstFunction() const
{
    return inFirstFunction;
}

// Visitor methods implementation

void Analyzer::visit(ast::Num &node)
{
    node.type = ast::BuiltInType::INT;
}

void Analyzer::visit(ast::NumB &node)
{
    if (node.value > 255)
    {
        output::errorByteTooLarge(node.line, node.value);
    }

    node.type = ast::BuiltInType::BYTE;
}

void Analyzer::visit(ast::String &node)
{
    node.type = ast::BuiltInType::STRING;
}

void Analyzer::visit(ast::Bool &node)
{
    node.type = ast::BuiltInType::BOOL;
}

void Analyzer::visit(ast::ID &node)
{
    auto symbolEntry = symbolTable.findEntry(node.value, false);
    if (!symbolEntry)
    {
        output::errorUndef(node.line, node.value);
        return;
    }
    node.type = symbolEntry->getType().empty() ? ast::BuiltInType::VOID : symbolEntry->getType().front();
}

void Analyzer::visit(ast::BinOp &node)
{
    node.left->accept(*this);
    node.right->accept(*this);

    auto leftType = node.left->type;
    auto rightType = node.right->type;
    auto leftId = dynamic_pointer_cast<ast::ID>(node.left);
    auto rightId = dynamic_pointer_cast<ast::ID>(node.right);

    if (leftId)
    {
        auto leftSymbolEntry = symbolTable.findEntry(leftId->value, false);
        if (!leftSymbolEntry)
        {
            output::errorUndef(node.line, leftId->value);
        }
        if (leftSymbolEntry->isArray())
        {
            output::errorMismatch(node.line);
        }
    }

    if (rightId)
    {
        auto rightSymbolEntry = symbolTable.findEntry(rightId->value, false);
        if (!rightSymbolEntry)
        {
            output::errorUndef(node.line, rightId->value);
        }
        if (rightSymbolEntry->isArray())
        {
            output::errorMismatch(node.line);
        }
    }

    if (leftType == ast::BuiltInType::BYTE && rightType == ast::BuiltInType::BYTE)
    {
        node.type = ast::BuiltInType::BYTE;
    }
    else if (leftType == ast::BuiltInType::INT && rightType == ast::BuiltInType::INT)
    {
        node.type = ast::BuiltInType::INT;
    }
    else
    {
        output::errorMismatch(node.line);
    }
}

void Analyzer::visit(ast::RelOp &node)
{
    node.left->accept(*this);
    node.right->accept(*this);

    auto leftType = node.left->type;
    auto rightType = node.right->type;

    auto leftID = std::dynamic_pointer_cast<ast::ID>(node.left);
    if (leftID)
    {
        auto entry = symbolTable.findEntry(leftID->value, false);
        if (!entry)
            output::errorUndef(node.line, leftID->value);
        if (entry->isArray())
            output::errorMismatch(node.line);
    }

    auto rightID = std::dynamic_pointer_cast<ast::ID>(node.right);
    if (rightID)
    {
        auto entry = symbolTable.findEntry(rightID->value, false);
        if (!entry)
            output::errorUndef(node.line, rightID->value);
        if (entry->isArray())
            output::errorMismatch(node.line);
    }

    // Types must be numeric
    if (leftType == ast::BuiltInType::BOOL || leftType == ast::BuiltInType::STRING ||
        rightType == ast::BuiltInType::BOOL || rightType == ast::BuiltInType::STRING)
    {
        output::errorMismatch(node.line);
    }

    node.type = ast::BuiltInType::BOOL;
}
