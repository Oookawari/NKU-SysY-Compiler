#ifndef __SYMBOLTABLE_H__
#define __SYMBOLTABLE_H__

#include <string>
#include <vector>
#include <map>

class Type;
class Operand;

class SymbolEntry
{
private:
    int kind;
protected:
    enum {CONSTANT, VARIABLE, TEMPORARY};
    Type *type;

public:
    SymbolEntry(Type *type, int kind);
    virtual ~SymbolEntry() {};
    bool isConstant() const {return kind == CONSTANT;};
    bool isTemporary() const {return kind == TEMPORARY;};
    bool isVariable() const {return kind == VARIABLE;};
    Type* getType() {return type;};
    void setType(Type *type) {this->type = type;};
    virtual std::string toStr() = 0;
    // You can add any function you need here.
};


/*  
    Symbol entry for literal constant. Example:

    int a = 1;

    Compiler should create constant symbol entry for literal constant '1'.
*/
class ConstantSymbolEntry : public SymbolEntry
{
private:
    int value;

public:
    ConstantSymbolEntry(Type *type, int value);
    virtual ~ConstantSymbolEntry() {};
    int getValue() const {return value;};
    Type* getType() const {return type;};
    std::string toStr();
    // You can add any function you need here.
};

class FloatConstantSymbolEntry : public SymbolEntry
{
private:
    float value;

public:
    FloatConstantSymbolEntry(Type *type, float value);
    virtual ~FloatConstantSymbolEntry() {};
    float getValue() const {return value;};
    Type* getType() const {return type;};
    std::string toStr();
};

/*
class FloatConstantSymbolEntry : public SymbolEntry
{
private:
    float value;
public:
    FloatConstantSymbolEntry(Type *type, float value);
    virtual ~FloatConstantSymbolEntry() {};
    float getValue() const {return value;};
    Type* getType() const {return type;};
    std::string toStr();
};
*/

/* 
    Symbol entry for identifier. Example:

    int a;
    int b;
    void f(int c)
    {
        int d;
        {
            int e;
        }
    }

    Compiler should create identifier symbol entries for variables a, b, c, d and e:

    | variable | scope    |
    | a        | GLOBAL   |
    | b        | GLOBAL   |
    | c        | PARAM    |
    | d        | LOCAL    |
    | e        | LOCAL +1 |
*/
class IdentifierSymbolEntry : public SymbolEntry
{
private:
    enum {GLOBAL, PARAM, LOCAL};
    std::string name;
    int scope;
    Operand *addr;  // The address of the identifier.
    Operand *param_addr;
    bool Const;
    // You can add any field you need here.
    bool isFunc;
    bool isArray;
    int label;
    int Const_Value = 0;
public:
    IdentifierSymbolEntry(Type *type, std::string name, int scope);
    IdentifierSymbolEntry(Type *type, std::string name, int scope, bool isConst);
    virtual ~IdentifierSymbolEntry() {};
    std::string toStr();
    bool isGlobal() const {return scope == GLOBAL;};
    bool isParam() const {return scope == PARAM;};
    bool isLocal() const {return scope >= LOCAL;};
    bool isConst() const {return Const;};
    int getScope() const {return scope;};
    void setAddr(Operand *addr) {this->addr = addr;};
    Operand* getAddr() {return addr;};
    void setParamAddr(Operand *param_addr) {this->param_addr = param_addr;};
    Operand* getParamAddr() {return param_addr;};
    // You can add any function you need here.
    bool setAsFunc() {isFunc = true;};
    bool setAsArray() {isArray = true;};
    bool setLabel(int label) {this->label = label;};
    int getConstValue(){return Const_Value;}
    void setConstValue(int Const_value){this->Const_Value = Const_value;}
};


/* 
    Symbol entry for temporary variable created by compiler. Example:

    int a;
    a = 1 + 2 + 3;

    The compiler would generate intermediate code like:

    t1 = 1 + 2
    t2 = t1 + 3
    a = t2

    So compiler should create temporary symbol entries for t1 and t2:

    | temporary variable | label |
    | t1                 | 1     |
    | t2                 | 2     |
*/
class TemporarySymbolEntry : public SymbolEntry
{
private:
    int stack_offset;
    int label;
public:
    TemporarySymbolEntry(Type *type, int label);
    virtual ~TemporarySymbolEntry() {};
    std::string toStr();
    int getLabel() const {return label;};
    void setOffset(int offset) { this->stack_offset = offset; };
    int getOffset() { return this->stack_offset; };
    // You can add any function you need here.
};

// symbol table managing identifier symbol entries
class SymbolTable
{
private:
    std::map<std::string, SymbolEntry*> symbolTable;
    SymbolTable *prev;
    int level;
    static int counter;
public:
    SymbolTable();
    SymbolTable(SymbolTable *prev);
    void install(std::string name, SymbolEntry* entry);
    SymbolEntry* lookup(std::string name);
    SymbolEntry* lookupLocalVariable(std::string name);
    SymbolEntry* lookupGlobalVariable(std::string name);//便于寻找函数
    SymbolTable* getPrev() {return prev;};
    int getLevel() {return level;};
    static int getLabel() {return counter++;};
};

extern SymbolTable *identifiers;
extern SymbolTable *globals;

#endif
