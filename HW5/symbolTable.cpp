#include "symbolTable.hpp"

#include <iostream>

// Implementing the SymbolEntry class methods

SymbolEntry::SymbolEntry(string name, vector<BuiltInType> type, bool is_func,
                         bool has_return, int entry_offset,
                         BuiltInType entry_return_type,
                         bool is_formal_parameter, bool is_array,
                         int array_size, const string &llvmRig)
    : entry_name(std::move(name)),
      entry_type(std::move(type)),
      is_func(is_func),
      has_return(has_return),
      entry_offset(entry_offset),
      entry_return_type(entry_return_type),
      is_formal_parameter(is_formal_parameter),
      is_array(is_array),
      array_size(array_size), llvmRig(llvmRig) {}

// Implementing getters for SymbolEntry

string SymbolEntry::getName() const { return entry_name; }

vector<BuiltInType> SymbolEntry::getType() const { return entry_type; }

bool SymbolEntry::isFunction() const { return is_func; }

bool SymbolEntry::isFormalParameter() const { return is_formal_parameter; }

bool SymbolEntry::hasReturn() const { return has_return; }

int SymbolEntry::getOffset() const { return entry_offset; }

BuiltInType SymbolEntry::getReturnType() const { return entry_return_type; }

bool SymbolEntry::isArray() const { return is_array; }

int SymbolEntry::getArraySize() const { return array_size; }

string SymbolEntry::getLlvmRig() const { return llvmRig; }

// Implementing setters for SymbolEntry

void SymbolEntry::setName(const string &name) { entry_name = name; }

void SymbolEntry::setType(const vector<BuiltInType> &type) {
    entry_type = move(type);
}

void SymbolEntry::setIsFunction(bool is_func) { this->is_func = is_func; }

void SymbolEntry::setHasReturn(bool has_return) {
    this->has_return = has_return;
}

void SymbolEntry::setOffset(int offset) { entry_offset = offset; }

void SymbolEntry::setReturnType(BuiltInType return_type) {
    entry_return_type = return_type;
}

void SymbolEntry::setIsFormalParameter(bool is_formal_parameter) {
    this->is_formal_parameter = is_formal_parameter;
}

void SymbolEntry::setIsArray(bool is_array) { this->is_array = is_array; }

void SymbolEntry::setArraySize(int size) { array_size = size; }

void SymbolEntry::setLlvmRig(const string &llvmRig) { this->llvmRig = llvmRig; }

// Implementing the Scope class methods

Scope::Scope(bool is_loop_scope) : is_loop_scope(is_loop_scope) {
    scope_entries = vector<shared_ptr<SymbolEntry>>();
}

void Scope::addSymbol(shared_ptr<SymbolEntry> entry) {
    scope_entries.push_back(move(entry));
}

shared_ptr<SymbolEntry> Scope::findEntry(const string &name) const {
    for (const auto &entry : scope_entries) {
        if (entry->getName() == name) {
            return entry;
        }
    }
    return nullptr;
}

bool Scope::contains(const string &name) const {
    for (const auto &entry : scope_entries) {
        if (entry->getName() == name) {
            return true;
        }
    }
    return false;
}

vector<shared_ptr<SymbolEntry>> Scope::getEntries() const {
    return scope_entries;
}

vector<BuiltInType> Scope::getFunctionArgumentTypes(const string &name) const {
    shared_ptr<SymbolEntry> entry = findEntry(name);
    if (entry && entry->isFunction()) {
        return entry->getType();
    }
    return vector<BuiltInType>();  // Return an empty vector if not found or not
                                   // a function
}

bool Scope::isLoopScope() const { return is_loop_scope; }

void Scope::setLoopScope(bool is_loopScope) {
    this->is_loop_scope = is_loopScope;
}

// Impleminting the SymbolTable class methods

SymbolTable::SymbolTable() : scopes(), symbolTable_offsets(0) {
    scopes.push_back(make_shared<Scope>());  // Initialize with a global scope
}

void SymbolTable::beginScope() {
    scopes.push_back(make_shared<Scope>());
    symbolTable_offsets.push_back(this->symbolTable_offsets.empty()
                                      ? 0
                                      : this->symbolTable_offsets.back());
}

void SymbolTable::endScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
        symbolTable_offsets.pop_back();
    }
}

void SymbolTable::addEntry(shared_ptr<SymbolEntry> entry) {
    if (entry->isFunction()) {
        scopes.front()->addSymbol(entry);
    } else if (entry->isFormalParameter()) {
        entry->setOffset(symbolTable_offsets.back()--);
        scopes.back()->addSymbol(entry);
    } else {
        int size = entry->isArray() ? entry->getArraySize() : 1;
        entry->setOffset(symbolTable_offsets.back());
        symbolTable_offsets.back() += size;
        scopes.back()->addSymbol(entry);
    }
}

shared_ptr<SymbolEntry> SymbolTable::findEntry(const string &name,
                                               bool is_function) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        shared_ptr<SymbolEntry> entry = (*it)->findEntry(name);
        if (entry && entry->isFunction() == is_function) {
            return entry;
        }
    }
    return nullptr;
}

bool SymbolTable::contains(const string &name, bool is_function) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        shared_ptr<SymbolEntry> entry = (*it)->findEntry(name);
        if (entry && entry->isFunction() == is_function) {
            return true;
        }
    }
    return false;
}

vector<shared_ptr<SymbolEntry>> SymbolTable::getCurrentScopeEntries() const {
    if (!scopes.empty()) {
        return scopes.back()->getEntries();
    }
    return vector<shared_ptr<SymbolEntry>>();
}

int SymbolTable::getOffset() const {
    return symbolTable_offsets.empty() ? 0 : symbolTable_offsets.back();
}

shared_ptr<Scope> SymbolTable::getLastScope() {
    if (!scopes.empty()) {
        return scopes.back();
    }
    return nullptr;
}

shared_ptr<Scope> SymbolTable::getGlobalScope() {
    if (!scopes.empty()) {
        return scopes.front();
    }
    return nullptr;
}

vector<BuiltInType> SymbolTable::getFunctionArgTypes(const string &name) {
    if (!scopes.empty()) {
        return scopes.back()->getFunctionArgumentTypes(name);
    }
    return vector<BuiltInType>();
}

vector<shared_ptr<Scope>> SymbolTable::getScopes() { return scopes; }

void SymbolTable::setOffset(int offset) { symbolTable_offsets.back() = offset; }

SymbolEntry::~SymbolEntry() {}
Scope::~Scope() {}
SymbolTable::~SymbolTable() {}
