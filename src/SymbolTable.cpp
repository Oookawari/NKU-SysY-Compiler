#include "SymbolTable.h"
#include <iostream>
#include <sstream>

SymbolEntry::SymbolEntry(Type *type, int kind) 
{
    this->type = type;
    this->kind = kind;
}

ConstantSymbolEntry::ConstantSymbolEntry(Type *type, int value) : SymbolEntry(type, SymbolEntry::CONSTANT)
{
    this->value = value;
}

std::string ConstantSymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << value;
    return buffer.str();
}

FloatConstantSymbolEntry::FloatConstantSymbolEntry(Type *type, float value) : SymbolEntry(type, SymbolEntry::CONSTANT)
{
    this->value = value;
}

std::string FloatConstantSymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << value;
    return buffer.str();
}

IdentifierSymbolEntry::IdentifierSymbolEntry(Type *type, std::string name, int scope) : SymbolEntry(type, SymbolEntry::VARIABLE), name(name)
{
    Const = false;
    this->scope = scope;
    addr = nullptr;
    param_addr = nullptr;
    isFunc = false;
    isArray = false;
    label = -1;
}

IdentifierSymbolEntry::IdentifierSymbolEntry(Type *type, std::string name, int scope, bool isConst) : SymbolEntry(type, SymbolEntry::VARIABLE), name(name)
{
    this->Const = isConst;
    this->scope = scope;
    addr = nullptr;
    param_addr = nullptr;
    isFunc = false;
    isArray = false;
    label = -1;
}

std::string IdentifierSymbolEntry::toStr()
{
    // std::ostringstream buffer;
    // if (label < 0) {
    //     if (type->isFunc())
    //         buffer << '@';
    //     buffer << name;
    // } else
    //     buffer << "%t" << label;
    // return buffer.str();

    std::ostringstream buffer;
    if (isFunc) {
        buffer << name;
    }else if(scope == 0){
        buffer << name;
    }
    else{
        buffer << "%t" << label;
    }
    return buffer.str();
}

TemporarySymbolEntry::TemporarySymbolEntry(Type *type, int label) : SymbolEntry(type, SymbolEntry::TEMPORARY)
{
    this->label = label;
}

std::string TemporarySymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << "%t" << label;
    return buffer.str();
}

SymbolTable::SymbolTable()
{
    prev = nullptr;
    level = 0;
}

SymbolTable::SymbolTable(SymbolTable *prev)
{
    this->prev = prev;
    this->level = prev->level + 1;
}

/*
    Description: lookup the symbol entry of an identifier in the symbol table
    Parameters: 
        name: identifier name
    Return: pointer to the symbol entry of the identifier

    hint:
    1. The symbol table is a stack. The top of the stack contains symbol entries in the current scope.
    2. Search the entry in the current symbol table at first.
    3. If it's not in the current table, search it in previous ones(along the 'prev' link).
    4. If you find the entry, return it.
    5. If you can't find it in all symbol tables, return nullptr.
*/
SymbolEntry* SymbolTable::lookup(std::string name)
{
    int exist = this->symbolTable.count(name);
    if(exist == 0) {
    	if(this->getLevel() != 0) {
    		return this->getPrev()->lookup(name);
    	}
    	else {
    return nullptr;
    	}
    } else {
    	return this->symbolTable[name];
    }
}

SymbolEntry* SymbolTable::lookupLocalVariable(std::string name)
{
    int exist = this->symbolTable.count(name);
    if(exist == 0) {
        return nullptr;
    } else {
    	return this->symbolTable[name];
    }
}

SymbolEntry* SymbolTable::lookupGlobalVariable(std::string name)
{
    if(this->getLevel() != 0){
        this->getPrev()->lookupGlobalVariable(name);
    }
    int exist = this->symbolTable.count(name);
    if(exist == 0) {
	    return nullptr;
    } else {
    	return this->symbolTable[name];
    }
}

// install the entry into current symbol table.
void SymbolTable::install(std::string name, SymbolEntry* entry)
{
    symbolTable[name] = entry;
}

int SymbolTable::counter = 0;
static SymbolTable t;
SymbolTable *identifiers = &t;
SymbolTable *globals = &t;
