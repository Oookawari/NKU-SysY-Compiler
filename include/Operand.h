#ifndef __OPERAND_H__
#define __OPERAND_H__

#include "SymbolTable.h"
#include <vector>
#include <iostream>
#include "Type.h"
class Instruction;
class Function;


// class Operand - The operand of an instruction.
class Operand
{
typedef std::vector<Instruction *>::iterator use_iterator;

protected:
    Instruction *def;                // The instruction where this operand is defined.
    std::vector<Instruction *> uses; // Intructions that use this operand.
    enum {SIMPLE, ZERO, TRUE, INITER};
    int Kind;
public:
    bool IsAddr = false;
    Operand() {def = nullptr;};
    void setDef(Instruction *inst) {def = inst;};
    void addUse(Instruction *inst) {uses.push_back(inst);};
    void removeUse(Instruction *inst);
    int usersNum() const {return uses.size();};
    bool IsSimple() {return Kind == SIMPLE;}
    bool IsZero() {return Kind == ZERO;}
    bool IsTrue() {return Kind == TRUE;}
    bool IsInit() {return Kind == INITER;}
    use_iterator use_begin() {return uses.begin();};
    use_iterator use_end() {return uses.end();};
    Instruction* getDef() { return def; };
    virtual Type* getType() = 0;
    virtual std::string toStr() = 0;
    virtual SymbolEntry* getEntry() = 0;
};

class SimpleOperand : public Operand
{
private:
    SymbolEntry *se;                 // The symbol entry of this operand.
public:
    SimpleOperand(SymbolEntry*se) : Operand(), se(se) {this->Kind = SIMPLE;};
    Type* getType() {return se->getType();}
    std::string toStr() {return se->toStr();};
    void setType(Type *type){se->setType(type);};
    SymbolEntry * getEntry() { return se; };
};

class ZeroOperand : public Operand
{
private:
    SymbolEntry *se;         
public:
    ZeroOperand() : Operand(){
        this->Kind = ZERO;
        se = new ConstantSymbolEntry(TypeSystem::intType, 0);
        };
    Type* getType() {return TypeSystem::intType;};
    SymbolEntry * getEntry() { return se; };
    std::string toStr() {return "0";};
};

class TrueOperand : public Operand
{
private:
    SymbolEntry *se;     
public:
    TrueOperand() : Operand(){
        this->Kind = TRUE;
        se = new ConstantSymbolEntry(TypeSystem::boolType, 0);
        };
    Type* getType() {return TypeSystem::boolType;};
    SymbolEntry * getEntry() { return se; };
    std::string toStr() {return "true";};
};

class Zeroinitializer : public Operand
{
private:
    SymbolEntry *se; 
    Type* type;    
public:
    Zeroinitializer(Type* tp) : Operand(){
        this->type = tp;
        this->Kind = INITER;
        se = new ConstantSymbolEntry(TypeSystem::boolType, 0);
        };
    Zeroinitializer() : Operand(){
        this->type = TypeSystem::boolType;
        this->Kind = INITER;
        se = new ConstantSymbolEntry(TypeSystem::boolType, 0);
        };
    Type* getType() {return type;};
    void setType(Type* type){this->type = type;}
    SymbolEntry * getEntry() { return se; };
    std::string toStr() {return "zeroinitializer";};
};

extern ZeroOperand *zero_oprand;
extern TrueOperand *true_oprand;
extern Zeroinitializer *zeroinitializer;
#endif