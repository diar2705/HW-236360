#include "analyzer.hpp"
#include <vector>

using namespace std;

// Helper functions

std::vector<std::string> builtInTypeToString(vector<ast::BuiltInType> types) {
    std::vector<std::string> result;
    for (const auto &type : types) {
        switch (type) {
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

int getArraySize(std::shared_ptr<ast::ArrayType> arrType) {
    if (auto num = std::dynamic_pointer_cast<ast::Num>(arrType->length)) {
        return num->value;
    } else if (auto numB =
                   std::dynamic_pointer_cast<ast::NumB>(arrType->length)) {
        return numB->value;
    }
    return -1;  // unreachable if semantic checks pass
}

vector<BuiltInType> getFormals(shared_ptr<ast::Formals> node) {
    vector<BuiltInType> result;
    for (auto formal : node->formals) {
        auto ptype =
            std::dynamic_pointer_cast<ast::PrimitiveType>(formal->type);
        result.push_back(ptype->type);
    }
    return result;
}

// Implementing Analyzer class methods

Analyzer::Analyzer()
    : symbolTable(),
      codeBuffer(),
      inFirstFunction(false),
      currentReturnType(ast::BuiltInType::VOID) {}

void Analyzer::printOutput()
{
    cout << codeBuffer << endl;
}

void Analyzer::setInFirstFunction(bool val) { inFirstFunction = val; }

bool Analyzer::getInFirstFunction() const { return inFirstFunction; }

ast::BuiltInType Analyzer::getCurrentReturnType() const {
    return currentReturnType;
}

void Analyzer::setCurrentReturnType(ast::BuiltInType type) {
    currentReturnType = type;
}

// Visitor methods implementation

void Analyzer::visit(ast::Num &node) {
    node.type = ast::BuiltInType::INT;
    node.reg = to_string(node.value);
}

void Analyzer::visit(ast::NumB &node) {
    if (node.value > 255 || node.value < 0) {
        output::errorByteTooLarge(node.line, node.value);
    }

    node.type = ast::BuiltInType::BYTE;
    node.reg = to_string(node.value);
}

void Analyzer::visit(ast::String &node) {
    node.type = ast::BuiltInType::STRING;
    node.reg = codeBuffer.emitString(node.value);
}

void Analyzer::visit(ast::Bool &node) {
    node.type = ast::BuiltInType::BOOL;
    node.reg = node.value ? "1" : "0";
}

void Analyzer::visit(ast::ID &node) {
    auto symbolEntry = symbolTable.findEntry(node.value, false);
    if (!symbolEntry) {
        // If not found as variable, check if it's a function
        if (symbolTable.contains(node.value, true)) {
            output::errorDefAsFunc(node.line, node.value);
        } else {
            output::errorUndef(node.line, node.value);
        }
        return;
    }
    node.type = symbolEntry->getType().empty() ? ast::BuiltInType::VOID
                                               : symbolEntry->getType().front();

    // Generate LLVM code to load the variable
    if (!symbolEntry->isArray()) {
        string tempReg = codeBuffer.freshVar();
        string llvmType = (node.type == ast::BuiltInType::INT)    ? "i32"
                          : (node.type == ast::BuiltInType::BYTE) ? "i8"
                          : (node.type == ast::BuiltInType::BOOL) ? "i1"
                                                                  : "i8*";

        codeBuffer << tempReg << " = load " << llvmType << ", " << llvmType
                   << "* " << symbolEntry->getLlvmRig() << endl;
        node.reg = tempReg;
    } else {
        // For arrays, we just store the pointer
        node.reg = symbolEntry->getLlvmRig();
    }
}

void Analyzer::visit(ast::BinOp &node) {
    node.left->accept(*this);
    node.right->accept(*this);

    auto leftType = node.left->type;
    auto rightType = node.right->type;
    auto leftId = dynamic_pointer_cast<ast::ID>(node.left);
    auto rightId = dynamic_pointer_cast<ast::ID>(node.right);

    if (leftId) {
        auto leftSymbolEntry = symbolTable.findEntry(leftId->value, false);
        if (!leftSymbolEntry) {
            output::errorUndef(node.line, leftId->value);
        }
        if (leftSymbolEntry && leftSymbolEntry->isArray()) {
            output::errorMismatch(node.line);
        }
    }

    if (rightId) {
        auto rightSymbolEntry = symbolTable.findEntry(rightId->value, false);
        if (!rightSymbolEntry) {
            output::errorUndef(node.line, rightId->value);
        }
        if (rightSymbolEntry && rightSymbolEntry->isArray()) {
            output::errorMismatch(node.line);
        }
    }

    if ((leftType == ast::BuiltInType::INT &&
         rightType == ast::BuiltInType::INT) ||
        (leftType == ast::BuiltInType::INT &&
         rightType == ast::BuiltInType::BYTE) ||
        (leftType == ast::BuiltInType::BYTE &&
         rightType == ast::BuiltInType::INT)) {
        node.type = ast::BuiltInType::INT;
    } else if (leftType == ast::BuiltInType::BYTE &&
               rightType == ast::BuiltInType::BYTE) {
        node.type = ast::BuiltInType::BYTE;
    } else {
        output::errorMismatch(node.line);
    }

    // Generate LLVM code for binary operation
    string leftReg = node.left->reg;
    string rightReg = node.right->reg;
    string resultReg = codeBuffer.freshVar();

    // Handle type promotions
    if (leftType == ast::BuiltInType::BYTE &&
        (rightType == ast::BuiltInType::INT ||
         node.type == ast::BuiltInType::INT)) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << leftReg << " to i32"
                   << endl;
        leftReg = extendedReg;
    }
    if (rightType == ast::BuiltInType::BYTE &&
        (leftType == ast::BuiltInType::INT ||
         node.type == ast::BuiltInType::INT)) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << rightReg << " to i32"
                   << endl;
        rightReg = extendedReg;
    }

    string llvmType = (node.type == ast::BuiltInType::INT) ? "i32" : "i8";
    string opCode;

    switch (node.op) {
        case ast::BinOpType::ADD:
            opCode = "add";
            break;
        case ast::BinOpType::SUB:
            opCode = "sub";
            break;
        case ast::BinOpType::MUL:
            opCode = "mul";
            break;
        case ast::BinOpType::DIV:
            // Add division by zero check
            if (node.op == ast::BinOpType::DIV) {
                string isZeroReg = codeBuffer.freshVar();
                string errorLabel = codeBuffer.freshLabel();
                string continueLabel = codeBuffer.freshLabel();

                // Check if divisor is zero
                codeBuffer << isZeroReg << " = icmp eq " << llvmType << " "
                           << rightReg << ", 0" << endl;
                codeBuffer << "br i1 " << isZeroReg << ", label " << errorLabel
                           << ", label " << continueLabel << endl;

                // Handle division by zero error
                codeBuffer.emitLabel(errorLabel);
                string errorMsg = "Error division by zero";
                string strVar = codeBuffer.emitString(errorMsg);
                // Don't hardcode the string length, use the actual length
                codeBuffer << "call void @print(i8* getelementptr (["
                           << errorMsg.length() + 1 << " x i8], ["
                           << errorMsg.length() + 1 << " x i8]* " << strVar
                           << ", i32 0, i32 0))" << endl;
                codeBuffer << "call void @exit(i32 0)" << endl;
                codeBuffer << "br label " << continueLabel << endl;

                // Continue with division
                codeBuffer.emitLabel(continueLabel);
            }
            opCode = "sdiv";
            break;
    }

    codeBuffer << resultReg << " = " << opCode << " " << llvmType << " "
               << leftReg << ", " << rightReg << endl;
    node.reg = resultReg;
}

void Analyzer::visit(ast::RelOp &node) {
    node.left->accept(*this);
    node.right->accept(*this);

    auto leftType = node.left->type;
    auto rightType = node.right->type;

    auto leftID = std::dynamic_pointer_cast<ast::ID>(node.left);
    if (leftID) {
        auto entry = symbolTable.findEntry(leftID->value, false);
        if (!entry)
            output::errorUndef(node.line, leftID->value);
        if (entry && entry->isArray())
            output::errorMismatch(node.line);
    }

    auto rightID = std::dynamic_pointer_cast<ast::ID>(node.right);
    if (rightID) {
        auto entry = symbolTable.findEntry(rightID->value, false);
        if (!entry)
            output::errorUndef(node.line, rightID->value);
        if (entry && entry->isArray())
            output::errorMismatch(node.line);
    }

    // Types must be numeric
    if (leftType == ast::BuiltInType::BOOL ||
        leftType == ast::BuiltInType::STRING ||
        rightType == ast::BuiltInType::BOOL ||
        rightType == ast::BuiltInType::STRING) {
        output::errorMismatch(node.line);
    }

    node.type = ast::BuiltInType::BOOL;

    // Generate LLVM code for relational operation
    string leftReg = node.left->reg;
    string rightReg = node.right->reg;
    string resultReg = codeBuffer.freshVar();

    // Handle type promotions
    if (leftType == ast::BuiltInType::BYTE &&
        rightType == ast::BuiltInType::INT) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << leftReg << " to i32"
                   << endl;
        leftReg = extendedReg;
        leftType = ast::BuiltInType::INT;
    }
    if (rightType == ast::BuiltInType::BYTE &&
        leftType == ast::BuiltInType::INT) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << rightReg << " to i32"
                   << endl;
        rightReg = extendedReg;
        rightType = ast::BuiltInType::INT;
    }

    string llvmType = (leftType == ast::BuiltInType::INT ||
                       rightType == ast::BuiltInType::INT)
                          ? "i32"
                          : "i8";
    string opCode;

    switch (node.op) {
        case ast::RelOpType::EQ:
            opCode = "icmp eq";
            break;
        case ast::RelOpType::NE:
            opCode = "icmp ne";
            break;
        case ast::RelOpType::LT:
            opCode = "icmp slt";
            break;
        case ast::RelOpType::GT:
            opCode = "icmp sgt";
            break;
        case ast::RelOpType::LE:
            opCode = "icmp sle";
            break;
        case ast::RelOpType::GE:
            opCode = "icmp sge";
            break;
    }

    codeBuffer << resultReg << " = " << opCode << " " << llvmType << " "
               << leftReg << ", " << rightReg << endl;
    node.reg = resultReg;
}

void Analyzer::visit(ast::Not &node) {
    node.exp->accept(*this);
    if (node.exp->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;

    // Generate LLVM code for NOT operation
    string resultReg = codeBuffer.freshVar();
    codeBuffer << resultReg << " = xor i1 " << node.exp->reg << ", 1" << endl;
    node.reg = resultReg;
}

void Analyzer::visit(ast::And &node) {
    node.left->accept(*this);
    node.right->accept(*this);

    if (node.left->type != ast::BuiltInType::BOOL ||
        node.right->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;

    // Generate LLVM code for AND operation
    string resultReg = codeBuffer.freshVar();
    codeBuffer << resultReg << " = and i1 " << node.left->reg << ", "
               << node.right->reg << endl;
    node.reg = resultReg;
}

void Analyzer::visit(ast::Or &node) {
    node.left->accept(*this);
    node.right->accept(*this);

    if (node.left->type != ast::BuiltInType::BOOL ||
        node.right->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;

    // Generate LLVM code for OR operation
    string resultReg = codeBuffer.freshVar();
    codeBuffer << resultReg << " = or i1 " << node.left->reg << ", "
               << node.right->reg << endl;
    node.reg = resultReg;
}

void Analyzer::visit(ast::PrimitiveType &node) {
    // Primitive types are already defined, no need to check anything
    node.type = node.type;
}

void Analyzer::visit(ast::ArrayType &node) {
    node.length->accept(*this);
    if (node.length->type != ast::BuiltInType::INT &&
        node.length->type != ast::BuiltInType::BYTE) {
        output::errorMismatch(node.line);
    }

    if (!std::dynamic_pointer_cast<ast::Num>(node.length) &&
        !std::dynamic_pointer_cast<ast::NumB>(node.length)) {
        output::errorMismatch(node.line);
    }
}

void Analyzer::visit(ast::ArrayDereference &node) {
    node.id->accept(*this);
    node.index->accept(*this);
    if (node.index->type != ast::BuiltInType::INT &&
        node.index->type != ast::BuiltInType::BYTE) {
        output::errorMismatch(node.line);
    }
    auto entry = symbolTable.findEntry(node.id->value, false);
    if (!entry) {
        output::errorUndef(node.line, node.id->value);
    }
    if (!entry->isArray()) {
        output::errorMismatch(node.line);
    }
    node.type = entry->getType()[0];

    // Generate LLVM code for array dereference
    string indexReg = node.index->reg;

    // Convert byte index to int if needed
    if (node.index->type == ast::BuiltInType::BYTE) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << indexReg << " to i32"
                   << endl;
        indexReg = extendedReg;
    }

    // Add bounds checking
    string isNegativeReg = codeBuffer.freshVar();
    string isOutOfBoundsReg = codeBuffer.freshVar();
    string combinedCheckReg = codeBuffer.freshVar();
    string errorLabel = codeBuffer.freshLabel();
    string continueLabel = codeBuffer.freshLabel();

    // Check if index is negative
    codeBuffer << isNegativeReg << " = icmp slt i32 " << indexReg << ", 0"
               << endl;

    // Check if index >= array size
    codeBuffer << isOutOfBoundsReg << " = icmp sge i32 " << indexReg << ", "
               << entry->getArraySize() << endl;

    // Combine the checks (true if out of bounds)
    codeBuffer << combinedCheckReg << " = or i1 " << isNegativeReg << ", "
               << isOutOfBoundsReg << endl;
    codeBuffer << "br i1 " << combinedCheckReg << ", label " << errorLabel
               << ", label " << continueLabel << endl;

    // Handle out of bounds error
    codeBuffer.emitLabel(errorLabel);
    string errorMsg = "Error out of bounds";
    string strVar = codeBuffer.emitString(errorMsg);
    // Don't hardcode the string length, use the actual length
    codeBuffer << "call void @print(i8* getelementptr (["
               << errorMsg.length() + 1 << " x i8], [" << errorMsg.length() + 1
               << " x i8]* " << strVar << ", i32 0, i32 0))" << endl;
    codeBuffer << "call void @exit(i32 0)" << endl;
    codeBuffer << "br label " << continueLabel << endl;

    // Continue with array access
    codeBuffer.emitLabel(continueLabel);

    string ptrReg = codeBuffer.freshVar();
    string resultReg = codeBuffer.freshVar();
    string llvmType = (node.type == BuiltInType::INT)    ? "i32"
                      : (node.type == BuiltInType::BYTE) ? "i8"
                      : (node.type == BuiltInType::BOOL) ? "i1"
                                                         : "i8*";

    codeBuffer << ptrReg << " = getelementptr " << llvmType << ", " << llvmType
               << "* " << entry->getLlvmRig() << ", i32 " << indexReg << endl;
    codeBuffer << resultReg << " = load " << llvmType << ", " << llvmType
               << "* " << ptrReg << endl;

    node.reg = resultReg;
}

void Analyzer::visit(ast::ArrayAssign &node) {
    node.id->accept(*this);
    node.index->accept(*this);
    node.exp->accept(*this);
    if (node.index->type != ast::BuiltInType::INT &&
        node.index->type != ast::BuiltInType::BYTE) {
        output::errorMismatch(node.line);
    }
    auto entry = symbolTable.findEntry(node.id->value, false);
    if (!entry) {
        output::errorUndef(node.line, node.id->value);
    }

    auto rhsID = std::dynamic_pointer_cast<ast::ID>(node.exp);
    if (rhsID) {
        auto rhsEntry = symbolTable.findEntry(rhsID->value, false);
        if (rhsEntry && rhsEntry->isArray()) {
            output::errorMismatch(node.line);  // RHS is an array → illegal
        }
    }

    if (!entry->isArray()) {
        output::errorMismatch(node.line);
    }
    ast::BuiltInType elemType = entry->getType()[0];
    if (elemType != node.exp->type &&
        !(elemType == ast::BuiltInType::INT &&
          node.exp->type == ast::BuiltInType::BYTE)) {
        output::errorMismatch(node.line);
    }

    // Generate LLVM code for array assignment
    string indexReg = node.index->reg;
    string valueReg = node.exp->reg;

    // Convert byte index to int if needed
    if (node.index->type == ast::BuiltInType::BYTE) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << indexReg << " to i32"
                   << endl;
        indexReg = extendedReg;
    }

    // Add bounds checking
    string isNegativeReg = codeBuffer.freshVar();
    string isOutOfBoundsReg = codeBuffer.freshVar();
    string combinedCheckReg = codeBuffer.freshVar();
    string errorLabel = codeBuffer.freshLabel();
    string continueLabel = codeBuffer.freshLabel();

    // Check if index is negative
    codeBuffer << isNegativeReg << " = icmp slt i32 " << indexReg << ", 0"
               << endl;

    // Check if index >= array size
    codeBuffer << isOutOfBoundsReg << " = icmp sge i32 " << indexReg << ", "
               << entry->getArraySize() << endl;

    // Combine the checks (true if out of bounds)
    codeBuffer << combinedCheckReg << " = or i1 " << isNegativeReg << ", "
               << isOutOfBoundsReg << endl;
    codeBuffer << "br i1 " << combinedCheckReg << ", label " << errorLabel
               << ", label " << continueLabel << endl;

    // Handle out of bounds error
    codeBuffer.emitLabel(errorLabel);
    string errorMsg = "Error out of bounds";
    string strVar = codeBuffer.emitString(errorMsg);
    // Don't hardcode the string length, use the actual length
    codeBuffer << "call void @print(i8* getelementptr (["
               << errorMsg.length() + 1 << " x i8], [" << errorMsg.length() + 1
               << " x i8]* " << strVar << ", i32 0, i32 0))" << endl;
    codeBuffer << "call void @exit(i32 0)" << endl;
    codeBuffer << "br label " << continueLabel << endl;

    // Continue with array assignment
    codeBuffer.emitLabel(continueLabel);

    // Handle type promotion for BYTE to INT if needed
    if (elemType == ast::BuiltInType::INT &&
        node.exp->type == ast::BuiltInType::BYTE) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << valueReg << " to i32"
                   << endl;
        valueReg = extendedReg;
    }

    string llvmType = (elemType == ast::BuiltInType::INT)    ? "i32"
                      : (elemType == ast::BuiltInType::BYTE) ? "i8"
                      : (elemType == ast::BuiltInType::BOOL) ? "i1"
                                                             : "i8*";

    string ptrReg = codeBuffer.freshVar();
    codeBuffer << ptrReg << " = getelementptr " << llvmType << ", " << llvmType
               << "* " << entry->getLlvmRig() << ", i32 " << indexReg << endl;
    codeBuffer << "store " << llvmType << " " << valueReg << ", " << llvmType
               << "* " << ptrReg << endl;
}

void Analyzer::visit(ast::Cast &node) {
    node.exp->accept(*this);
    node.target_type->accept(*this);
    auto expType = node.exp->type;
    auto targetType = node.target_type->type;
    if ((expType != ast::BuiltInType::INT &&
         expType != ast::BuiltInType::BYTE) ||
        (targetType != ast::BuiltInType::INT &&
         targetType != ast::BuiltInType::BYTE)) {
        output::errorMismatch(node.line);
    }
    node.type = targetType;

    // Generate LLVM code for cast
    if (expType == targetType) {
        // No cast needed
        node.reg = node.exp->reg;
    } else if (expType == ast::BuiltInType::BYTE &&
               targetType == ast::BuiltInType::INT) {
        // Zero extend byte to int
        string resultReg = codeBuffer.freshVar();
        codeBuffer << resultReg << " = zext i8 " << node.exp->reg << " to i32"
                   << endl;
        node.reg = resultReg;
    } else if (expType == ast::BuiltInType::INT &&
               targetType == ast::BuiltInType::BYTE) {
        // Truncate int to byte
        string resultReg = codeBuffer.freshVar();
        codeBuffer << resultReg << " = trunc i32 " << node.exp->reg << " to i8"
                   << endl;
        node.reg = resultReg;
    }
}

void Analyzer::visit(ast::ExpList &node) {
    for (auto &exp : node.exps) {
        exp->accept(*this);
    }
}

void Analyzer::visit(ast::Call &node) {
    // node.func_id->accept(*this);
    node.args->accept(*this);

    // If same name is defined as a variable, raise error
    if (symbolTable.contains(node.func_id->value, false)) {
        output::errorDefAsVar(node.line, node.func_id->value);
    }

    // Check function existence
    if (!symbolTable.contains(node.func_id->value, true)) {
        output::errorUndefFunc(node.line, node.func_id->value);
    }

    auto entry = symbolTable.findEntry(node.func_id->value, true);
    auto expArgTypes = entry->getType();
    auto args = node.args->exps;

    // Check if the number of arguments matches the function prototype
    if (expArgTypes.size() != args.size()) {
        auto expectedTypes = builtInTypeToString(expArgTypes);
        output::errorPrototypeMismatch(node.line, node.func_id->value,
                                       expectedTypes);
    } else {
        for (size_t i = 0; i < args.size(); ++i) {
            ast::BuiltInType actualArgType = args[i]->type;
            ast::BuiltInType expectedParamType = expArgTypes[i];

            bool mismatch = false;

            // Handle type compatibility: BYTE can be promoted to INT
            if (expectedParamType == ast::BuiltInType::INT &&
                actualArgType == ast::BuiltInType::BYTE) {
                mismatch = false;  // BYTE to INT is allowed
            } else if (expectedParamType != actualArgType) {
                mismatch = true;
            }

            // Check if an entire array is being passed where a primitive is
            // expected.
            auto argAsID = std::dynamic_pointer_cast<ast::ID>(args[i]);
            if (argAsID) {
                auto symbolEntryForID =
                    symbolTable.findEntry(argAsID->value, false);
                if (symbolEntryForID && symbolEntryForID->isArray()) {
                    mismatch = true;
                }
            }

            if (mismatch) {
                auto expectedTypes = builtInTypeToString(expArgTypes);
                output::errorPrototypeMismatch(node.line, node.func_id->value,
                                               expectedTypes);
            }
        }
    }
    // Set the type of the call node to the function's return type
    node.type = entry->getReturnType();

    // Generate LLVM code for function call
    string argList = "";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0)
            argList += ", ";

        string argReg = args[i]->reg;
        ast::BuiltInType argType = args[i]->type;
        ast::BuiltInType expectedType = expArgTypes[i];

        // Handle string arguments - need getelementptr
        if (argType == ast::BuiltInType::STRING) {
            auto stringNode = std::dynamic_pointer_cast<ast::String>(args[i]);
            if (stringNode) {
                string ptrReg = codeBuffer.freshVar();
                int strLen = stringNode->value.length() + 1;
                codeBuffer << ptrReg << " = getelementptr [" << strLen
                           << " x i8], [" << strLen << " x i8]* " << argReg
                           << ", i32 0, i32 0" << endl;
                argReg = ptrReg;
            }
        }

        // Handle type promotion
        if (argType == ast::BuiltInType::BYTE &&
            expectedType == ast::BuiltInType::INT) {
            string extendedReg = codeBuffer.freshVar();
            codeBuffer << extendedReg << " = zext i8 " << argReg << " to i32"
                       << endl;
            argReg = extendedReg;
            argType = ast::BuiltInType::INT;
        }

        string llvmType = (argType == ast::BuiltInType::INT)    ? "i32"
                          : (argType == ast::BuiltInType::BYTE) ? "i8"
                          : (argType == ast::BuiltInType::BOOL) ? "i1"
                                                                : "i8*";
        argList += llvmType + " " + argReg;
    }

    if (entry->getReturnType() == ast::BuiltInType::VOID) {
        codeBuffer << "call void @" << node.func_id->value << "(" << argList
                   << ")" << endl;
        node.reg = "";
    } else {
        string resultReg = codeBuffer.freshVar();
        string retType =
            (entry->getReturnType() == ast::BuiltInType::INT)    ? "i32"
            : (entry->getReturnType() == ast::BuiltInType::BYTE) ? "i8"
            : (entry->getReturnType() == ast::BuiltInType::BOOL) ? "i1"
                                                                 : "i8*";
        codeBuffer << resultReg << " = call " << retType << " @"
                   << node.func_id->value << "(" << argList << ")" << endl;
        node.reg = resultReg;
    }
}

void Analyzer::visit(ast::Statements &node) {
    bool isScopeOpen = false;
    if (!inFirstFunction) {
        symbolTable.beginScope();
        isScopeOpen = true;
    }

    setInFirstFunction(false);

    for (auto &stmt : node.statements) {
        stmt->accept(*this);
    }

    if (isScopeOpen) {
        symbolTable.endScope();
    }
}

void Analyzer::visit(ast::Break &node) {
    for (auto scope : symbolTable.getScopes()) {
        if (scope->isLoopScope()) {
            return;  // Found a loop scope, break is valid
        }
    }
    output::errorUnexpectedBreak(node.line);
}

void Analyzer::visit(ast::Continue &node) {
    for (auto scope : symbolTable.getScopes()) {
        if (scope->isLoopScope()) {
            return;  // Found a loop scope, continue is valid
        }
    }
    output::errorUnexpectedContinue(node.line);
}

void Analyzer::visit(ast::Return &node) {
    if (!node.exp) {
        if (currentReturnType != ast::BuiltInType::VOID) {
            output::errorMismatch(node.line);
        }
        // Generate LLVM code for void return
        codeBuffer << "ret void" << endl;
        return;
    }

    node.exp->accept(*this);
    bool mismatch = true;

    if (currentReturnType == ast::BuiltInType::INT &&
        node.exp->type == ast::BuiltInType::BYTE) {
        mismatch = false;  // BYTE to INT is allowed
    } else if (node.exp->type == currentReturnType) {
        mismatch = false;
    }

    // Check if an array is being returned where a primitive type is expected
    auto returnAsID = std::dynamic_pointer_cast<ast::ID>(node.exp);
    if (returnAsID) {
        auto symbolEntryForReturn =
            symbolTable.findEntry(returnAsID->value, false);
        if (symbolEntryForReturn && symbolEntryForReturn->isArray()) {
            mismatch = true;
        }
    }

    if (mismatch) {
        output::errorMismatch(node.line);
    }

    // Generate LLVM code for return with value
    string retReg = node.exp->reg;
    string llvmType = (currentReturnType == BuiltInType::INT)    ? "i32"
                      : (currentReturnType == BuiltInType::BYTE) ? "i8"
                      : (currentReturnType == BuiltInType::BOOL) ? "i1"
                                                                 : "i8*";

    // Handle type promotion
    if (currentReturnType == ast::BuiltInType::INT &&
        node.exp->type == ast::BuiltInType::BYTE) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << retReg << " to i32"
                   << endl;
        retReg = extendedReg;
    }

    codeBuffer << "ret " << llvmType << " " << retReg << endl;
}

void Analyzer::visit(ast::If &node) {
    node.condition->accept(*this);
    if (node.condition->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }

    // Generate LLVM code for if statement
    string thenLabel = codeBuffer.freshLabel();
    string elseLabel = codeBuffer.freshLabel();
    string endLabel = codeBuffer.freshLabel();

    // Conditional branch
    if (node.otherwise) {
        codeBuffer << "br i1 " << node.condition->reg << ", label " << thenLabel
                   << ", label " << elseLabel << endl;
    } else {
        codeBuffer << "br i1 " << node.condition->reg << ", label " << thenLabel
                   << ", label " << endLabel << endl;
    }

    // Then block
    codeBuffer.emitLabel(thenLabel);
    symbolTable.beginScope();
    node.then->accept(*this);
    symbolTable.endScope();
    codeBuffer << "br label " << endLabel << endl;

    // Else block (if exists)
    if (node.otherwise) {
        codeBuffer.emitLabel(elseLabel);
        symbolTable.beginScope();
        node.otherwise->accept(*this);
        symbolTable.endScope();
        codeBuffer << "br label " << endLabel << endl;
    }

    // End label
    codeBuffer.emitLabel(endLabel);
}

void Analyzer::visit(ast::While &node) {
    // Generate LLVM code for while loop
    string condLabel = codeBuffer.freshLabel();
    string bodyLabel = codeBuffer.freshLabel();
    string endLabel = codeBuffer.freshLabel();

    // Jump to condition check
    codeBuffer << "br label " << condLabel << endl;

    // Condition check
    codeBuffer.emitLabel(condLabel);
    node.condition->accept(*this);
    if (node.condition->type != ast::BuiltInType::BOOL) {
        output::errorMismatch(node.line);
    }
    codeBuffer << "br i1 " << node.condition->reg << ", label " << bodyLabel
               << ", label " << endLabel << endl;

    // Loop body
    codeBuffer.emitLabel(bodyLabel);
    symbolTable.beginScope();
    symbolTable.getLastScope()->setLoopScope(true);  // Mark this scope as a loop scope
    node.body->accept(*this);
    symbolTable.endScope();
    codeBuffer << "br label " << condLabel << endl;  // Jump back to condition

    // End label
    codeBuffer.emitLabel(endLabel);
}

void Analyzer::visit(ast::VarDecl &node) {
    node.type->accept(*this);
    // node.id->accept(*this);

    if (symbolTable.contains(node.id->value, false) ||
        symbolTable.contains(node.id->value, true)) {
        output::errorDef(node.line, node.id->value);
    }

    auto arrType = std::dynamic_pointer_cast<ast::ArrayType>(node.type);
    auto primType = std::dynamic_pointer_cast<ast::PrimitiveType>(node.type);

    BuiltInType expectedType;
    if (primType) {
        expectedType = primType->type;
    } else if (arrType) {
        expectedType = arrType->type;
    }

    if (node.init_exp) {
        node.init_exp->accept(*this);

        auto initID = std::dynamic_pointer_cast<ast::ID>(node.init_exp);
        // Check if init_exp is an array
        if (initID) {
            auto initEntry = symbolTable.findEntry(initID->value, false);
            if (initEntry && initEntry->isArray()) {
                output::errorMismatch(node.line);
            }
        }

        // Check if init_exp is just an identifier;
        if (initID) {
            if (!symbolTable.contains(initID->value, false)) {
                if (symbolTable.contains(initID->value, true)) {
                    output::errorDefAsFunc(node.line, initID->value);
                }
                output::errorUndef(node.line, initID->value);
            }
        }

        auto initCall = std::dynamic_pointer_cast<ast::Call>(node.init_exp);
        if (initCall) {
            auto funcEntry =
                symbolTable.findEntry(initCall->func_id->value, true);
            if (funcEntry->getReturnType() != expectedType) {
                output::errorMismatch(node.line);
            }
        } else {
            if (expectedType != node.init_exp->type &&
                !(expectedType == BuiltInType::INT &&
                  node.init_exp->type == BuiltInType::BYTE)) {
                output::errorMismatch(node.line);
            }
        }
    }

    // Generate LLVM code for variable declaration
    string varReg = codeBuffer.freshVar();
    string llvmType;

    if (arrType) {
        arrType->accept(*this);
        int size = getArraySize(arrType);
        llvmType = (arrType->type == ast::BuiltInType::INT)    ? "i32"
                   : (arrType->type == ast::BuiltInType::BYTE) ? "i8"
                   : (arrType->type == ast::BuiltInType::BOOL) ? "i1"
                                                               : "i8*";

        codeBuffer << varReg << " = alloca " << llvmType << ", i32 " << size
                   << endl;

        // Initialize array elements to 0
        string zeroValue;
        if (arrType->type == ast::BuiltInType::INT)
            zeroValue = "0";
        else if (arrType->type == ast::BuiltInType::BYTE)
            zeroValue = "0";
        else if (arrType->type == ast::BuiltInType::BOOL)
            zeroValue = "0";
        else  // String
            zeroValue = "null";

        // Loop through array and initialize each element to zero
        string indexVar = codeBuffer.freshVar();
        string loopStart = codeBuffer.freshLabel();
        string loopBody = codeBuffer.freshLabel();
        string loopEnd = codeBuffer.freshLabel();
        string elemPtrVar = codeBuffer.freshVar();

        // Initialize loop counter
        codeBuffer << indexVar << " = alloca i32" << endl;
        codeBuffer << "store i32 0, i32* " << indexVar << endl;
        codeBuffer << "br label " << loopStart << endl;

        // Loop condition check
        codeBuffer.emitLabel(loopStart);
        string currentIndex = codeBuffer.freshVar();
        string conditionVar = codeBuffer.freshVar();
        codeBuffer << currentIndex << " = load i32, i32* " << indexVar << endl;
        codeBuffer << conditionVar << " = icmp slt i32 " << currentIndex << ", "
                   << size << endl;
        codeBuffer << "br i1 " << conditionVar << ", label " << loopBody
                   << ", label " << loopEnd << endl;

        // Loop body - set element to zero
        codeBuffer.emitLabel(loopBody);
        string reloadedIndex = codeBuffer.freshVar();
        codeBuffer << reloadedIndex << " = load i32, i32* " << indexVar << endl;
        codeBuffer << elemPtrVar << " = getelementptr " << llvmType << ", "
                   << llvmType << "* " << varReg << ", i32 " << reloadedIndex
                   << endl;
        codeBuffer << "store " << llvmType << " " << zeroValue << ", "
                   << llvmType << "* " << elemPtrVar << endl;

        // Increment loop counter
        string incrementedIndex = codeBuffer.freshVar();
        codeBuffer << incrementedIndex << " = add i32 " << reloadedIndex
                   << ", 1" << endl;
        codeBuffer << "store i32 " << incrementedIndex << ", i32* " << indexVar
                   << endl;
        codeBuffer << "br label " << loopStart << endl;

        // Loop exit
        codeBuffer.emitLabel(loopEnd);

        symbolTable.addEntry(std::make_shared<SymbolEntry>(
            node.id->value, std::vector<ast::BuiltInType>{arrType->type}, false,
            false, 0, BuiltInType::VOID, false, true, size, varReg));
    } else {
        llvmType = (expectedType == ast::BuiltInType::INT)    ? "i32"
                   : (expectedType == ast::BuiltInType::BYTE) ? "i8"
                   : (expectedType == ast::BuiltInType::BOOL) ? "i1"
                                                              : "i8*";

        codeBuffer << varReg << " = alloca " << llvmType << endl;

        symbolTable.addEntry(std::make_shared<SymbolEntry>(
            node.id->value, std::vector<ast::BuiltInType>{expectedType}, false,
            false, 0, BuiltInType::VOID, false, false, 0, varReg));

        // Initialize the variable if there's an init expression
        if (node.init_exp) {
            string initReg = node.init_exp->reg;

            // Handle type promotion
            if (expectedType == ast::BuiltInType::INT &&
                node.init_exp->type == ast::BuiltInType::BYTE) {
                string extendedReg = codeBuffer.freshVar();
                codeBuffer << extendedReg << " = zext i8 " << initReg
                           << " to i32" << endl;
                initReg = extendedReg;
            }

            codeBuffer << "store " << llvmType << " " << initReg << ", "
                       << llvmType << "* " << varReg << endl;
        } else {
            // Initialize primitive variables to 0
            string zeroValue;
            if (expectedType == ast::BuiltInType::INT)
                zeroValue = "0";
            else if (expectedType == ast::BuiltInType::BYTE)
                zeroValue = "0";
            else if (expectedType == ast::BuiltInType::BOOL)
                zeroValue = "0";
            else  // String
                zeroValue = "null";

            codeBuffer << "store " << llvmType << " " << zeroValue << ", "
                       << llvmType << "* " << varReg << endl;
        }
    }
}

void Analyzer::visit(ast::Assign &node) {
    node.exp->accept(*this);
    ast::BuiltInType rhsType = node.exp->type;

    node.id->accept(*this);

    auto lhsID = std::dynamic_pointer_cast<ast::ID>(node.id);
    std::shared_ptr<SymbolEntry> lhsEntry = nullptr;

    if (lhsID) {
        lhsEntry = symbolTable.findEntry(lhsID->value, false);
    } else if (auto lhsArrayDeref =
                   std::dynamic_pointer_cast<ast::ArrayDereference>(node.id)) {
        lhsEntry = symbolTable.findEntry(lhsArrayDeref->id->value, false);
    }

    if (!lhsEntry) {
        if (lhsID && symbolTable.contains(lhsID->value, true)) {
            output::errorDefAsFunc(node.line, lhsID->value);
        } else if (lhsID)  // If it's an ID and not found as var or func
        {
            output::errorUndef(node.line, lhsID->value);
        } else {
            // This case might happen if node.id is not an ID or
            // ArrayDereference,
            output::errorMismatch(node.line);  // Generic mismatch as a fallback
        }
    }

    ast::BuiltInType lhsResolvedType;
    if (std::dynamic_pointer_cast<ast::ArrayDereference>(node.id)) {
        lhsResolvedType = node.id->type;
    } else {
        // It's a simple ID
        lhsResolvedType = lhsEntry->getType()[0];
    }

    // LHS is an array variable itself
    if (lhsEntry && lhsEntry->isArray() &&
        !std::dynamic_pointer_cast<ast::ArrayDereference>(node.id)) {
        output::ErrorInvalidAssignArray(node.line, lhsEntry->getName());
    }

    // RHS is an array variable being assigned to a primitive LHS
    auto rhsAsID = std::dynamic_pointer_cast<ast::ID>(node.exp);
    if (rhsAsID) {
        auto rhsEntry = symbolTable.findEntry(rhsAsID->value, false);
        if (rhsEntry && rhsEntry->isArray()) {
            output::errorMismatch(node.line);
            return;
        }
    }

    // BYTE can be implicitly converted to INT. Other types must match exactly.
    if (lhsResolvedType != rhsType &&
        !(lhsResolvedType == ast::BuiltInType::INT &&
          rhsType == ast::BuiltInType::BYTE)) {
        output::errorMismatch(node.line);
    }

    // Generate LLVM code for assignment
    string rhsReg = node.exp->reg;
    string lhsPtr;

    // Handle type promotion
    if (lhsResolvedType == ast::BuiltInType::INT &&
        rhsType == ast::BuiltInType::BYTE) {
        string extendedReg = codeBuffer.freshVar();
        codeBuffer << extendedReg << " = zext i8 " << rhsReg << " to i32"
                   << endl;
        rhsReg = extendedReg;
    }

    string llvmType = (lhsResolvedType == ast::BuiltInType::INT)    ? "i32"
                      : (lhsResolvedType == ast::BuiltInType::BYTE) ? "i8"
                      : (lhsResolvedType == ast::BuiltInType::BOOL) ? "i1"
                                                                    : "i8*";

    if (auto arrayDeref =
            std::dynamic_pointer_cast<ast::ArrayDereference>(node.id)) {
        // Array element assignment - need to get the pointer first
        string indexReg = arrayDeref->index->reg;

        // Convert byte index to int if needed
        if (arrayDeref->index->type == ast::BuiltInType::BYTE) {
            string extendedReg = codeBuffer.freshVar();
            codeBuffer << extendedReg << " = zext i8 " << indexReg << " to i32"
                       << endl;
            indexReg = extendedReg;
        }

        string ptrReg = codeBuffer.freshVar();
        codeBuffer << ptrReg << " = getelementptr " << llvmType << ", "
                   << llvmType << "* " << lhsEntry->getLlvmRig() << ", i32 "
                   << indexReg << endl;
        lhsPtr = ptrReg;
    } else {
        // Simple variable assignment
        lhsPtr = lhsEntry->getLlvmRig();
    }

    codeBuffer << "store " << llvmType << " " << rhsReg << ", " << llvmType
               << "* " << lhsPtr << endl;
}

void Analyzer::visit(ast::Formal &node) {
    // Check for illegal array type in parameters
    auto arrType = std::dynamic_pointer_cast<ast::ArrayType>(node.type);
    if (arrType) {
        output::errorMismatch(node.line);
    }

    auto primType = std::dynamic_pointer_cast<ast::PrimitiveType>(node.type);
    if (!primType) {
        output::errorMismatch(node.line);
    }

    ast::BuiltInType expectedType = primType->type;

    node.id->accept(*this);
    node.type->accept(*this);
}

void Analyzer::visit(ast::Formals &node) {
    for (auto &formal : node.formals) {
        formal->accept(*this);
    }
}

void Analyzer::visit(ast::FuncDecl &node) {
    node.return_type->accept(*this);

    auto funcEntry = symbolTable.findEntry(node.id->value, true);
    auto expArgTypes = funcEntry->getType();
    auto primReturnType =
        std::dynamic_pointer_cast<ast::PrimitiveType>(node.return_type);
    if (!primReturnType) {
        auto expectedTypes = builtInTypeToString(expArgTypes);
        output::errorPrototypeMismatch(
            node.line, node.id->value,
            expectedTypes);  // function return type must be primitive
    }

    BuiltInType returnType = primReturnType->type;
    vector<ast::BuiltInType> formalTypes;
    for (const auto &formal : node.formals->formals) {
        auto formalPrimType =
            std::dynamic_pointer_cast<ast::PrimitiveType>(formal->type);
        if (!formalPrimType) {
            output::errorMismatch(
                formal->line);  // formal parameter must be primitive
        }
        formalTypes.push_back(formalPrimType->type);
    }

    const auto &funcName = node.id->value;

    // Generate LLVM function declaration
    string retTypeStr = (returnType == ast::BuiltInType::VOID)   ? "void"
                        : (returnType == ast::BuiltInType::INT)  ? "i32"
                        : (returnType == ast::BuiltInType::BYTE) ? "i8"
                        : (returnType == ast::BuiltInType::BOOL) ? "i1"
                                                                 : "i8*";

    string paramList = "";
    for (size_t i = 0; i < formalTypes.size(); ++i) {
        if (i > 0)
            paramList += ", ";
        string paramType = (formalTypes[i] == ast::BuiltInType::INT)    ? "i32"
                           : (formalTypes[i] == ast::BuiltInType::BYTE) ? "i8"
                           : (formalTypes[i] == ast::BuiltInType::BOOL) ? "i1"
                                                                        : "i8*";
        paramList += paramType;
    }

    codeBuffer << "define " << retTypeStr << " @" << funcName << "("
               << paramList << ") {" << endl;

    // opening a new scope for function body
    symbolTable.beginScope();

    // setting formal parameters in the symbol table
    int oldOffset = symbolTable.getOffset();
    symbolTable.setOffset(-1);

    // Generate LLVM code for formal parameters - allocate space for each
    // parameter
    for (size_t i = 0; i < node.formals->formals.size(); ++i) {
        auto &formal = node.formals->formals[i];
        string paramReg = codeBuffer.freshVar();
        string llvmType = (formalTypes[i] == BuiltInType::INT)    ? "i32"
                          : (formalTypes[i] == BuiltInType::BYTE) ? "i8"
                          : (formalTypes[i] == BuiltInType::BOOL) ? "i1"
                                                                  : "i8*";

        codeBuffer << paramReg << " = alloca " << llvmType << endl;
        codeBuffer << "store " << llvmType << " %" << i << ", " << llvmType
                   << "* " << paramReg << endl;

        // Add the formal parameter to symbol table with LLVM register
        symbolTable.addEntry(std::make_shared<SymbolEntry>(
            formal->id->value, std::vector<ast::BuiltInType>{formalTypes[i]},
            false, false, 0, BuiltInType::VOID, true /* isFormalParameter */,
            false /* isArray */, 0 /* size */, paramReg));
    }

    node.formals->accept(*this);
    symbolTable.setOffset(oldOffset);  // restore offset

    BuiltInType previousReturn = currentReturnType;
    currentReturnType = returnType;

    setInFirstFunction(true);
    node.body->accept(*this);

    currentReturnType = previousReturn;

    // Add implicit return for void functions
    if (returnType == ast::BuiltInType::VOID) {
        codeBuffer << "ret void" << endl;
    }

    symbolTable.endScope();

    codeBuffer << "}" << endl << endl;
}

void Analyzer::visit(ast::Funcs &node) {
    bool mainFound = false;
    bool mainValid = false;

    // Emit the print functions at the beginning
    codeBuffer.emit("declare i32 @scanf(i8*, ...)\n");
    codeBuffer.emit("declare i32 @printf(i8*, ...)\n");
    codeBuffer.emit("declare void @exit(i32)\n");
    codeBuffer.emit("@.int_specifier_scan = constant [3 x i8] c\"%d\\00\"\n");
    codeBuffer.emit("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"\n");
    codeBuffer.emit("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"\n\n");

    codeBuffer.emit("define i32 @readi(i32) {\n");
    codeBuffer.emit("    %ret_val = alloca i32\n");
    codeBuffer.emit(
        "    %spec_ptr = getelementptr [3 x i8], [3 x i8]* "
        "@.int_specifier_scan, i32 0, i32 0\n");
    codeBuffer.emit(
        "    call i32 (i8*, ...) @scanf(i8* %spec_ptr, i32* %ret_val)\n");
    codeBuffer.emit("    %val = load i32, i32* %ret_val\n");
    codeBuffer.emit("    ret i32 %val\n");
    codeBuffer.emit("}\n\n");

    codeBuffer.emit("define void @printi(i32) {\n");
    codeBuffer.emit(
        "    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, "
        "i32 0, i32 0\n");
    codeBuffer.emit("    call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)\n");
    codeBuffer.emit("    ret void\n");
    codeBuffer.emit("}\n\n");

    codeBuffer.emit("define void @print(i8*) {\n");
    codeBuffer.emit(
        "    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, "
        "i32 0, i32 0\n");
    codeBuffer.emit("    call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)\n");
    codeBuffer.emit("    ret void\n");
    codeBuffer.emit("}\n\n");

    symbolTable.beginScope();
    symbolTable.addEntry(std::make_shared<SymbolEntry>(
        "print", std::vector<ast::BuiltInType>{ast::BuiltInType::STRING}, true,
        false, 0, ast::BuiltInType::VOID, false, false, 0));

    symbolTable.addEntry(std::make_shared<SymbolEntry>(
        "printi", std::vector<ast::BuiltInType>{ast::BuiltInType::INT}, true,
        false, 0, ast::BuiltInType::VOID, false, false, 0));
    
    for (auto &func : node.funcs) {
        if (func->id->value == "main") {
            mainFound = true;
            auto retType = std::dynamic_pointer_cast<ast::PrimitiveType>(
                func->return_type);
            if (retType && func->formals->formals.empty() &&
                retType->type == ast::BuiltInType::VOID) {
                mainValid = true;
            }
        }
        if (symbolTable.contains(func->id->value, true) ||
            symbolTable.contains(func->id->value, false)) {
            output::errorDef(func->line, func->id->value);
        }

        auto retType =
            std::dynamic_pointer_cast<ast::PrimitiveType>(func->return_type);
        if (!retType) {
            output::errorMismatch(
                func->line);  // function must return primitive
        }

        symbolTable.addEntry(std::make_shared<SymbolEntry>(
            func->id->value, getFormals(func->formals), true, true, 0,
            retType->type, false, false, 0));
    }
    if (!mainFound || !mainValid) {
        output::errorMainMissing();
    }
    for (auto &func : node.funcs) {
        func->accept(*this);
    }
    symbolTable.endScope();
}
