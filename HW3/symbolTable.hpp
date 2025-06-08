#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <string>
#include <vector>
#include <memory>
#include "nodes.hpp"
using namespace std;
using namespace ast;

class SymbolEntry
{
private:
    string entry_name;
    vector<BuiltInType> entry_type;
    bool is_func;
    bool has_return;
    int entry_offset;
    BuiltInType entry_return_type;
    bool is_formal_parameter;
    bool is_array;
    int array_size;

public:
    // Constructor for SymbolEntry
    SymbolEntry(string name, vector<BuiltInType> type, bool is_func = false,
                bool has_return = false, int entry_offset = 0,
                BuiltInType entry_return_type = BuiltInType::VOID, bool is_formal_parameter = false,
                bool is_array = false, int array_size = 0);

    // Getters for SymbolEntry
    string getName() const;
    vector<BuiltInType> getType() const;
    bool isFunction() const;
    bool isFormalParameter() const;
    bool hasReturn() const;
    int getOffset() const;
    BuiltInType getReturnType() const;
    bool isFormalParameter() const;
    bool isArray() const;
    int getArraySize() const;

    // Setters for SymbolEntry
    void setName(const string &name);
    void setType(const vector<BuiltInType> &type);
    void setIsFunction(bool is_func);
    void setHasReturn(bool has_return);
    void setOffset(int offset);
    void setReturnType(BuiltInType return_type);
    void setIsFormalParameter(bool is_formal_parameter);
    void setIsArray(bool is_array);
    void setArraySize(int size);

    ~SymbolEntry();
};

class Scope
{
private:
    vector<shared_ptr<SymbolEntry>> scope_entries;
    bool is_loop_scope;

public:
    Scope(bool is_loop_scope = false);

    // Add a symbol entry to the scope
    void addSymbol(shared_ptr<SymbolEntry> entry);

    // Find a symbol entry by name
    shared_ptr<SymbolEntry> findEntry(const string &name) const;

    // Check if the scope contains a symbol entry with the given name
    bool contains(const string &name) const;

    // Get all entries in the scope
    vector<shared_ptr<SymbolEntry>> getEntries() const;

    // Get the argument types of a function by its name
    vector<BuiltInType> getFunctionArgumentTypes(const string &name) const;

    // Check if this is a loop scope
    bool isLoopScope() const;

    // Set the loop scope status
    void setLoopScope(bool is_loop_scope);

    ~Scope();
};

class SymbolTable
{
private:
    vector<shared_ptr<Scope>> scopes;
    vector<int> symbolTable_offsets;

public:
    // Constructor for SymbolTable, initializes with a global scope
    SymbolTable();

    // Begin and end a scope
    void beginScope();
    void endScope();

    // Add a symbol entry to the current scope
    void addEntry(shared_ptr<SymbolEntry> entry);

    // Find a symbol entry by name in the current scope or any enclosing scope
    shared_ptr<SymbolEntry> findEntry(const string &name, bool is_function) const;

    // Check if a symbol entry with the given name exists in any scope
    bool contains(const string &name, bool is_function) const;

    // Get all entries in the current scope
    vector<shared_ptr<SymbolEntry>> getCurrentScopeEntries() const;

    // Get the offset of the current scope
    int getOffset() const;

    // Get the last scope, which is the current scope
    shared_ptr<Scope> getLastScope();

    // Get the global scope, which is the first scope in the symbol table
    shared_ptr<Scope> getGlobalScope();

    // Get the argument types of a function by its symbol name
    vector<BuiltInType> getFunctionArgTypes(const string &name);

    // Get all scopes in the symbol table
    vector<shared_ptr<Scope>> getScopes();

    // Set the offset for the current scope
    void setOffset(int offset);

    // Destructor for SymbolTable
    ~SymbolTable();
};

#endif // SYMBOL_TABLE_HPP