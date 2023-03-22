#ifndef __TYPE_H__
#define __TYPE_H__
#include <vector>
#include <string>
#include "SymbolTable.h"

class Type
{
private:
    int kind;
protected:
    enum {INT, VOID, FUNC, FLOAT, BOOL, PTR, ARRAY};
public:
    Type(int kind) : kind(kind) {};
    virtual ~Type() {};
    virtual std::string toStr() = 0;
    bool isInt() const {return kind == INT;};
    bool isVoid() const {return kind == VOID;};
    bool isFunc() const {return kind == FUNC;};
    bool isFloat() const {return kind == FLOAT;};
    bool isBool() const {return kind == BOOL;};
    bool isPTR() const {return kind == PTR;};
    bool isArray() const {return kind == ARRAY;};
    bool operator==(const Type &ano)
    {
        return(this->kind == ano.kind);
    }
};

class IntType : public Type
{
private:
    int size;
public:
    IntType(int size) : Type(Type::INT), size(size){};
    int getSize() {return size;};
    std::string toStr();
};

class FloatType : public Type
{
private:
    int size;
public:
    FloatType(int size) : Type(Type::FLOAT), size(size){};
    int getSize() {return size;};
    std::string toStr();
};

class BoolType : public Type
{
public:
    BoolType() : Type(Type::BOOL){};
    
    std::string toStr();
};

class VoidType : public Type
{
public:
    VoidType() : Type(Type::VOID){};
    std::string toStr();
};

class ArrayType : public Type
{
private: 
    Type* array_item_type;
    int item_count;
public:
    int getSize() {
        if(this->array_item_type->isInt()) return 4;
        else if(this->array_item_type->isArray()) {
            ArrayType* ar_tp = (ArrayType* )array_item_type;
            return ar_tp->getSize()*ar_tp->getItemCount();
        }
    };
    ArrayType(Type* array_item_type, int item_count = -1) : Type(Type::ARRAY) {
        this->array_item_type = array_item_type;
        this->item_count = item_count;
    };
    std::string toStr();
    Type* getItemType(){
        return array_item_type;
    };
    int getItemCount(){
        return item_count;
    }
    void setItemCount(int count) {item_count = count;};
};

class FunctionType : public Type
{
private:
    Type *returnType;
    std::vector<Type*> paramsType;
    std::vector<SymbolEntry*> paramsSe;
public:
    FunctionType(Type* returnType, 
        std::vector<Type*> paramsType, 
        std::vector<SymbolEntry*> paramsSe) : 
        Type(Type::FUNC), returnType(returnType), paramsType(paramsType), paramsSe(paramsSe){};
    Type *getRetType() {return returnType;};
    std::vector<Type*> getParamsType() {return paramsType;};
    void push_param_type(Type* type){paramsType.push_back(type);};
    std::vector<SymbolEntry*> getParamsSe() { return paramsSe; };
    void push_param_se(SymbolEntry* se){paramsSe.push_back(se);};
    std::string toStr();
};

class PointerType : public Type
{
private:
    Type *valueType;
public:
    PointerType(Type* valueType) : Type(Type::PTR) {this->valueType = valueType;};
    Type* getptType(){return valueType;}
    std::string toStr();
};

class TypeSystem
{
private:
    static IntType commonInt;
    static BoolType commonBool;
    static VoidType commonVoid;
    static FloatType commonFloat;
    //static BoolType commonBool;
public:
    static Type *intType;
    static Type *voidType;
    static Type *floatType;
    static Type *boolType;
};

#endif
