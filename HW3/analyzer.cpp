#include "analyzer.hpp"
#include <vector>

using namespace std;

// Helper functions

std::vector<std::string> builtInTypeToString(vector<ast::BuiltInType> types)
{
    std::vector<std::string> result;
    for (const auto &type : types)
    {
        switch (type)
        {
        case ast::BuiltInType::VOID:
            result.push_back("VOID");
            break;
        case ast::BuiltInType::BOOL:
            result.push_back("BOOL");
            break;
        case ast::BuiltInType::BYTE:
            result.push_back("BYTE");
            break;
        case ast::BuiltInType::INT:
            result.push_back("INT");
            break;
        case ast::BuiltInType::STRING:
            result.push_back("STRING");
            break;
        }
    }
    return result;
}

int getArraySize(std::shared_ptr<ast::ArrayType> arrType)
{
    if (auto num = std::dynamic_pointer_cast<ast::Num>(arrType->length))
    {
        return num->value;
    }
    else if (auto numB = std::dynamic_pointer_cast<ast::NumB>(arrType->length))
    {
        return numB->value;
    }
    return -1; // unreachable if semantic checks pass
}

vector<BuiltInType> getFormals(shared_ptr<ast::Formals> node)
{
    vector<BuiltInType> result;
    for (auto formal : node->formals)
    {
        auto ptype = std::dynamic_pointer_cast<ast::PrimitiveType>(formal->type);
        result.push_back(ptype->type);
    }
    return result;
}

// Implementing Analyzer class methods

Analyzer::Analyzer() : symbolTable(), printer(), inFirstFunction(false), currentReturnType(ast::BuiltInType::VOID) {}

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

ast::BuiltInType Analyzer::getCurrentReturnType() const
{
    return currentReturnType;
}

void Analyzer::setCurrentReturnType(ast::BuiltInType type)
{
    currentReturnType = type;
}

// Visitor methods implementation

void Analyzer::visit(ast::Num &node)
{
    node.type = ast::BuiltInType::INT;
}

void Analyzer::visit(ast::NumB &node)
{
    if (node.value > 255 || node.value < 0)
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
        // If not found as variable, check if it's a function
        if (symbolTable.contains(node.value, true))
        {
            output::errorDefAsFunc(node.line, node.value);
        }
        else
        {
            output::errorUndef(node.line, node.value);
        }
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

    if ((leftType == ast::BuiltInType::INT && rightType == ast::BuiltInType::INT) ||
        (leftType == ast::BuiltInType::INT && rightType == ast::BuiltInType::BYTE) ||
        (leftType == ast::BuiltInType::BYTE && rightType == ast::BuiltInType::INT))
    {
        node.type = ast::BuiltInType::INT;
    }
    else if (leftType == ast::BuiltInType::BYTE && rightType == ast::BuiltInType::BYTE)
    {
        node.type = ast::BuiltInType::BYTE;
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

void Analyzer::visit(ast::Not &node)
{
    node.exp->accept(*this);
    if (node.exp->type != ast::BuiltInType::BOOL)
    {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;
}

void Analyzer::visit(ast::And &node)
{
    node.left->accept(*this);
    node.right->accept(*this);

    if (node.left->type != ast::BuiltInType::BOOL || node.right->type != ast::BuiltInType::BOOL)
    {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;
}

void Analyzer::visit(ast::Or &node)
{
    node.left->accept(*this);
    node.right->accept(*this);

    if (node.left->type != ast::BuiltInType::BOOL || node.right->type != ast::BuiltInType::BOOL)
    {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;
}

void Analyzer::visit(ast::PrimitiveType &node)
{
    // Primitive types are already defined, no need to check anything
    node.type = node.type;
}

void Analyzer::visit(ast::ArrayType &node)
{
    node.length->accept(*this);
    if (node.length->type != ast::BuiltInType::INT && node.length->type != ast::BuiltInType::BYTE)
    {
        output::errorMismatch(node.line);
    }

    if (!std::dynamic_pointer_cast<ast::Num>(node.length) && !std::dynamic_pointer_cast<ast::NumB>(node.length))
    {
        output::errorMismatch(node.line);
    }
}

void Analyzer::visit(ast::ArrayDereference &node)
{
    node.id->accept(*this);
    node.index->accept(*this);
    if (node.index->type != ast::BuiltInType::INT && node.index->type != ast::BuiltInType::BYTE)
    {
        output::errorMismatch(node.line);
    }
    auto entry = symbolTable.findEntry(node.id->value, false);
    if (!entry)
    {
        output::errorUndef(node.line, node.id->value);
    }
    if (!entry->isArray())
    {
        output::errorMismatch(node.line);
    }
    node.type = entry->getType()[0];
}

void Analyzer::visit(ast::ArrayAssign &node)
{
    node.id->accept(*this);
    node.index->accept(*this);
    node.exp->accept(*this);
    if (node.index->type != ast::BuiltInType::INT && node.index->type != ast::BuiltInType::BYTE)
    {
        output::errorMismatch(node.line);
    }
    auto entry = symbolTable.findEntry(node.id->value, false);
    if (!entry)
    {
        output::errorUndef(node.line, node.id->value);
    }

    auto rhsID = std::dynamic_pointer_cast<ast::ID>(node.exp);
    if (rhsID)
    {
        auto rhsEntry = symbolTable.findEntry(rhsID->value, false);
        if (rhsEntry && rhsEntry->isArray())
        {
            output::errorMismatch(node.line); // RHS is an array → illegal
        }
    }

    if (!entry->isArray())
    {
        output::errorMismatch(node.line);
    }
    ast::BuiltInType elemType = entry->getType()[0];
    if (elemType != node.exp->type && !(elemType == ast::BuiltInType::INT && node.exp->type == ast::BuiltInType::BYTE))
    {
        output::errorMismatch(node.line);
    }
}

void Analyzer::visit(ast::Cast &node)
{
    node.exp->accept(*this);
    node.target_type->accept(*this);
    auto expType = node.exp->type;
    auto targetType = node.target_type->type;
    if ((expType != ast::BuiltInType::INT && expType != ast::BuiltInType::BYTE) ||
        (targetType != ast::BuiltInType::INT && targetType != ast::BuiltInType::BYTE))
    {
        output::errorMismatch(node.line);
    }
    node.type = targetType;
}

void Analyzer::visit(ast::ExpList &node)
{
    for (auto &exp : node.exps)
    {
        exp->accept(*this);
    }
}

void Analyzer::visit(ast::Call &node)
{
    // node.func_id->accept(*this);
    node.args->accept(*this);

    // If same name is defined as a variable, raise error
    if (symbolTable.contains(node.func_id->value, false))
    {
        output::errorDefAsVar(node.line, node.func_id->value);
    }

    // Check function existence
    if (!symbolTable.contains(node.func_id->value, true))
    {
        output::errorUndefFunc(node.line, node.func_id->value);
    }

    auto entry = symbolTable.findEntry(node.func_id->value, true);
    auto expArgTypes = entry->getType();
    auto args = node.args->exps;

    // Check if the number of arguments matches the function prototype
    if (expArgTypes.size() != args.size())
    {
        auto expectedTypes = builtInTypeToString(expArgTypes);
        output::errorPrototypeMismatch(node.line, node.func_id->value, expectedTypes);
    }
    else
    {
        for (size_t i = 0; i < args.size(); ++i)
        {
            ast::BuiltInType actualArgType = args[i]->type;
            ast::BuiltInType expectedParamType = expArgTypes[i];

            bool mismatch = false;

            // Handle type compatibility: BYTE can be promoted to INT
            if (expectedParamType == ast::BuiltInType::INT && actualArgType == ast::BuiltInType::BYTE)
            {
                mismatch = false; // BYTE to INT is allowed
            }
            else if (expectedParamType != actualArgType)
            {
                mismatch = true;
            }

            // Check if an entire array is being passed where a primitive is expected.
            auto argAsID = std::dynamic_pointer_cast<ast::ID>(args[i]);
            if (argAsID)
            {
                auto symbolEntryForID = symbolTable.findEntry(argAsID->value, false);
                if (symbolEntryForID && symbolEntryForID->isArray())
                {
                    mismatch = true;
                }
            }

            if (mismatch)
            {
                auto expectedTypes = builtInTypeToString(expArgTypes);
                output::errorPrototypeMismatch(node.line, node.func_id->value, expectedTypes);
            }
        }
    }
    // Set the type of the call node to the function's return type
    node.type = entry->getReturnType();
}

void Analyzer::visit(ast::Statements &node)
{
    bool isScopeOpen = false;
    if (!inFirstFunction)
    {
        symbolTable.beginScope();
        printer.beginScope();
        isScopeOpen = true;
    }

    setInFirstFunction(false);

    for (auto &stmt : node.statements)
    {
        stmt->accept(*this);
    }

    if (isScopeOpen)
    {
        symbolTable.endScope();
        printer.endScope();
    }
}

void Analyzer::visit(ast::Break &node)
{
    for (auto scope : symbolTable.getScopes())
    {
        if (scope->isLoopScope())
        {
            return; // Found a loop scope, break is valid
        }
    }
    output::errorUnexpectedBreak(node.line);
}

void Analyzer::visit(ast::Continue &node)
{
    for (auto scope : symbolTable.getScopes())
    {
        if (scope->isLoopScope())
        {
            return; // Found a loop scope, continue is valid
        }
    }
    output::errorUnexpectedContinue(node.line);
}

void Analyzer::visit(ast::Return &node)
{
    if (!node.exp)
    {
        if (currentReturnType != ast::BuiltInType::VOID)
        {
            output::errorMismatch(node.line);
        }
        return;
    }

    node.exp->accept(*this);
    bool mismatch = true;

    if (currentReturnType == ast::BuiltInType::INT &&
        node.exp->type == ast::BuiltInType::BYTE)
    {
        mismatch = false; // BYTE to INT is allowed
    }
    else if (node.exp->type == currentReturnType)
    {
        mismatch = false;
    }

    // Check if an array is being returned where a primitive type is expected
    auto returnAsID = std::dynamic_pointer_cast<ast::ID>(node.exp);
    if (returnAsID)
    {
        auto symbolEntryForReturn = symbolTable.findEntry(returnAsID->value, false);
        if (symbolEntryForReturn && symbolEntryForReturn->isArray())
        {
            mismatch = true;
        }
    }

    if (mismatch)
    {
        output::errorMismatch(node.line);
    }
}

void Analyzer::visit(ast::If &node)
{
    node.condition->accept(*this);
    if (node.condition->type != ast::BuiltInType::BOOL)
    {
        output::errorMismatch(node.line);
    }

    symbolTable.beginScope();
    printer.beginScope();
    node.then->accept(*this);
    symbolTable.endScope();
    printer.endScope();

    // If there's an 'otherwise' part, we need to handle it too
    if (node.otherwise)
    {
        symbolTable.beginScope();
        printer.beginScope();
        node.otherwise->accept(*this);
        symbolTable.endScope();
        printer.endScope();
    }
}

void Analyzer::visit(ast::While &node)
{
    node.condition->accept(*this);
    if (node.condition->type != ast::BuiltInType::BOOL)
    {
        output::errorMismatch(node.line);
    }

    symbolTable.beginScope();
    printer.beginScope();
    symbolTable.getLastScope()->setLoopScope(true); // Mark this scope as a loop scope
    node.body->accept(*this);
    symbolTable.endScope();
    printer.endScope();
}

void Analyzer::visit(ast::VarDecl &node)
{
    node.type->accept(*this);
    // node.id->accept(*this);

    if (symbolTable.contains(node.id->value, false) || symbolTable.contains(node.id->value, true))
    {
        output::errorDef(node.line, node.id->value);
    }

    auto arrType = std::dynamic_pointer_cast<ast::ArrayType>(node.type);
    auto primType = std::dynamic_pointer_cast<ast::PrimitiveType>(node.type);

    BuiltInType expectedType;
    if (primType)
    {
        expectedType = primType->type;
    }
    else if (arrType)
    {
        expectedType = arrType->type;
    }

    if (node.init_exp)
    {
        node.init_exp->accept(*this);

        auto initID = std::dynamic_pointer_cast<ast::ID>(node.init_exp);
        // Check if init_exp is an array
        if (initID)
        {
            auto initEntry = symbolTable.findEntry(initID->value, false);
            if (initEntry && initEntry->isArray())
            {
                output::errorMismatch(node.line);
            }
        }

        // Check if init_exp is just an identifier;
        if (initID)
        {
            if (!symbolTable.contains(initID->value, false))
            {
                if (symbolTable.contains(initID->value, true))
                {
                    output::errorDefAsFunc(node.line, initID->value);
                }
                output::errorUndef(node.line, initID->value);
            }
        }

        auto initCall = std::dynamic_pointer_cast<ast::Call>(node.init_exp);
        if (initCall)
        {
            auto funcEntry = symbolTable.findEntry(initCall->func_id->value, true);
            if (funcEntry->getReturnType() != expectedType)
            {
                output::errorMismatch(node.line);
            }
        }
        else
        {
            if (expectedType != node.init_exp->type &&
                !(expectedType == BuiltInType::INT && node.init_exp->type == BuiltInType::BYTE))
            {
                output::errorMismatch(node.line);
            }
        }
    }

    // Symbol insertion & code generation
    if (arrType)
    {
        arrType->accept(*this);
        int size = getArraySize(arrType);
        symbolTable.addEntry(std::make_shared<SymbolEntry>(node.id->value,
                                                           std::vector<ast::BuiltInType>{arrType->type},
                                                           false, false, 0, BuiltInType::VOID,
                                                           false, true, size));
        printer.emitArr(node.id->value, arrType->type, size,
                        symbolTable.findEntry(node.id->value, false)->getOffset());
    }
    else
    {
        symbolTable.addEntry(std::make_shared<SymbolEntry>(node.id->value,
                                                           std::vector<ast::BuiltInType>{expectedType},
                                                           false, false, 0, BuiltInType::VOID));
        printer.emitVar(node.id->value, expectedType,
                        symbolTable.findEntry(node.id->value, false)->getOffset());
    }
}

void Analyzer::visit(ast::Assign &node)
{
    node.exp->accept(*this);
    ast::BuiltInType rhsType = node.exp->type;

    node.id->accept(*this);

    auto lhsID = std::dynamic_pointer_cast<ast::ID>(node.id);
    std::shared_ptr<SymbolEntry> lhsEntry = nullptr;

    if (lhsID)
    {
        lhsEntry = symbolTable.findEntry(lhsID->value, false);
    }
    else if (auto lhsArrayDeref = std::dynamic_pointer_cast<ast::ArrayDereference>(node.id))
    {
        lhsEntry = symbolTable.findEntry(lhsArrayDeref->id->value, false);
    }

    if (!lhsEntry)
    {
        if (lhsID && symbolTable.contains(lhsID->value, true))
        {
            output::errorDefAsFunc(node.line, lhsID->value);
        }
        else if (lhsID) // If it's an ID and not found as var or func
        {
            output::errorUndef(node.line, lhsID->value);
        }
        else
        {
            // This case might happen if node.id is not an ID or ArrayDereference,
            output::errorMismatch(node.line); // Generic mismatch as a fallback
        }
    }

    ast::BuiltInType lhsResolvedType;
    if (std::dynamic_pointer_cast<ast::ArrayDereference>(node.id))
    {
        lhsResolvedType = node.id->type;
    }
    else
    {
        // It's a simple ID
        lhsResolvedType = lhsEntry->getType()[0];
    }

    // LHS is an array variable itself
    if (lhsEntry->isArray() && !std::dynamic_pointer_cast<ast::ArrayDereference>(node.id))
    {
        output::ErrorInvalidAssignArray(node.line, lhsEntry->getName());
    }

    // RHS is an array variable being assigned to a primitive LHS
    auto rhsAsID = std::dynamic_pointer_cast<ast::ID>(node.exp);
    if (rhsAsID)
    {
        auto rhsEntry = symbolTable.findEntry(rhsAsID->value, false);
        if (rhsEntry && rhsEntry->isArray())
        {
            output::errorMismatch(node.line);
            return;
        }
    }

    // BYTE can be implicitly converted to INT. Other types must match exactly.
    if (lhsResolvedType != rhsType && !(lhsResolvedType == ast::BuiltInType::INT && rhsType == ast::BuiltInType::BYTE))
    {
        output::errorMismatch(node.line);
    }
}

void Analyzer::visit(ast::Formal &node)
{

    if (symbolTable.contains(node.id->value, false) || symbolTable.contains(node.id->value, true))
    {
        output::errorDef(node.line, node.id->value);
    }

    // Check for illegal array type in parameters
    auto arrType = std::dynamic_pointer_cast<ast::ArrayType>(node.type);
    if (arrType)
    {
        output::errorMismatch(node.line);
    }

    auto primType = std::dynamic_pointer_cast<ast::PrimitiveType>(node.type);
    if (!primType)
    {
        output::errorMismatch(node.line);
    }

    ast::BuiltInType expectedType = primType->type;

    // Add to symbol table as a formal parameter
    symbolTable.addEntry(std::make_shared<SymbolEntry>(
        node.id->value,
        std::vector<ast::BuiltInType>{expectedType},
        false, false, 0, BuiltInType::VOID,
        true /* isFormalParameter */, false /* isArray */, 0 /* size */));

    // Print declaration info
    int offset = symbolTable.findEntry(node.id->value, false)->getOffset();
    printer.emitVar(node.id->value, expectedType, offset);

    node.id->accept(*this);
    node.type->accept(*this);
}

void Analyzer::visit(ast::Formals &node)
{
    for (auto &formal : node.formals)
    {
        formal->accept(*this);
    }
}

void Analyzer::visit(ast::FuncDecl &node)
{
    node.return_type->accept(*this);

    auto funcEntry = symbolTable.findEntry(node.id->value, true);
    auto expArgTypes = funcEntry->getType();
    auto primReturnType = std::dynamic_pointer_cast<ast::PrimitiveType>(node.return_type);
    if (!primReturnType)
    {
        auto expectedTypes = builtInTypeToString(expArgTypes);
        output::errorPrototypeMismatch(node.line, node.id->value, expectedTypes); // function return type must be primitive
    }

    BuiltInType returnType = primReturnType->type;
    vector<ast::BuiltInType> formalTypes;
    for (const auto &formal : node.formals->formals)
    {
        auto formalPrimType = std::dynamic_pointer_cast<ast::PrimitiveType>(formal->type);
        if (!formalPrimType)
        {
            output::errorMismatch(formal->line); // formal parameter must be primitive
        }
        formalTypes.push_back(formalPrimType->type);
    }

    const auto &funcName = node.id->value;

    // opening a new scope for function body
    symbolTable.beginScope();
    printer.beginScope();

    // setting formal parameters in the symbol table
    int oldOffset = symbolTable.getOffset();
    symbolTable.setOffset(-1);
    node.formals->accept(*this);
    symbolTable.setOffset(oldOffset); // restore offset

    BuiltInType previousReturn = currentReturnType;
    currentReturnType = returnType;

    setInFirstFunction(true);
    node.body->accept(*this);

    currentReturnType = previousReturn;

    symbolTable.endScope();
    printer.endScope();
}

void Analyzer::visit(ast::Funcs &node)
{
    bool mainFound = false;
    bool mainValid = false;
    symbolTable.beginScope();
    symbolTable.addEntry(std::make_shared<SymbolEntry>("print", std::vector<ast::BuiltInType>{ast::BuiltInType::STRING},
                                                       true, false, 0, ast::BuiltInType::VOID, false, false, 0));
    printer.emitFunc("print", ast::BuiltInType::VOID, std::vector<ast::BuiltInType>{ast::BuiltInType::STRING});

    symbolTable.addEntry(std::make_shared<SymbolEntry>("printi", std::vector<ast::BuiltInType>{ast::BuiltInType::INT},
                                                       true, false, 0, ast::BuiltInType::VOID, false, false, 0));
    printer.emitFunc("printi", ast::BuiltInType::VOID, std::vector<ast::BuiltInType>{ast::BuiltInType::INT});

    for (auto &func : node.funcs)
    {
        if (func->id->value == "main")
        {
            mainFound = true;
            auto retType = std::dynamic_pointer_cast<ast::PrimitiveType>(func->return_type);
            if (retType && func->formals->formals.empty() && retType->type == ast::BuiltInType::VOID)
            {
                mainValid = true;
            }
        }
        if (symbolTable.contains(func->id->value, true) || symbolTable.contains(func->id->value, false))
        {
            output::errorDef(func->line, func->id->value);
        }

        auto retType = std::dynamic_pointer_cast<ast::PrimitiveType>(func->return_type);
        if (!retType)
        {
            output::errorMismatch(func->line); // function must return primitive
        }

        symbolTable.addEntry(std::make_shared<SymbolEntry>(
            func->id->value, getFormals(func->formals), true, true, 0, retType->type, false, false, 0));
        printer.emitFunc(func->id->value, retType->type, getFormals(func->formals));
    }
    if (!mainFound || !mainValid)
    {
        output::errorMainMissing();
    }
    for (auto &func : node.funcs)
    {
        func->accept(*this);
    }
    symbolTable.endScope();
}
