#ifndef __AST_H__
#define __AST_H__

#include <fstream>
#include "Operand.h"
#include <string.h>
#include <vector>
#include <iostream>
class SymbolEntry;
class Unit;
class Function;
class BasicBlock;
class Instruction;
class IRBuilder;

class Node
{
private:
    static int counter;
    int seq;
protected:
    std::vector<Instruction*> true_list;
    std::vector<Instruction*> false_list;
    static IRBuilder *builder;
    void backPatch(std::vector<Instruction*> &list, BasicBlock*bb, bool true_branch);
    std::vector<Instruction*> merge(std::vector<Instruction*> &list1, std::vector<Instruction*> &list2);

public:
    Node();
    int getSeq() const {return seq;};
    static void setIRBuilder(IRBuilder*ib) {builder = ib;};
    virtual void output(int level) = 0;
    virtual void typeCheck() = 0;
    virtual void genCode() = 0;
    std::vector<Instruction*>& trueList() {return true_list;}
    std::vector<Instruction*>& falseList() {return false_list;}
};

class ExprNode : public Node
{
protected:
    SymbolEntry *symbolEntry;
    Operand *dst;   // The result of the subtree is stored into dst.
public:
    virtual int ComputeValue() = 0;
    ExprNode(SymbolEntry *symbolEntry) : symbolEntry(symbolEntry){};
    Operand* getOperand() {return dst;};
    SymbolEntry* getSymPtr() {return symbolEntry;};
    
};



class BinaryExpr : public ExprNode //Node for Binary operation
{
private:
    int op;
    ExprNode *expr1, *expr2;
public:
    enum {ADD, SUB, MULT, DIV, MOD, AND, OR, LESS, MORE, LESSEQUAL, MOREEQUAL, EQUAL, NOTEQUAL, LOR, LAND};
    BinaryExpr(SymbolEntry *se, int op, ExprNode*expr1, ExprNode*expr2) : ExprNode(se), op(op), expr1(expr1), expr2(expr2){dst = new SimpleOperand(se);};
    void output(int level);
    void typeCheck();
    void genCode();
    int ComputeValue();
};

class UnaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr1;
public:
    enum {NOT, PAREN, MINUS, PLUS};
    UnaryExpr(SymbolEntry *se, int op, ExprNode*expr1) : ExprNode(se), op(op), expr1(expr1){
        if(op == PAREN || op == PLUS)
            dst = expr1->getOperand(); 
        else
            dst = new SimpleOperand(se);
    };
    void output(int level);
    void typeCheck();
    void genCode();
    int ComputeValue();
};

class Cond : public ExprNode
{
private:
    ExprNode* lexpr;
public:
    Cond(SymbolEntry *se, ExprNode* lexpr) : ExprNode(se), lexpr(lexpr) {};
    void output(int level);
    void typeCheck();
    void genCode();
    int ComputeValue();
};

class Constant : public ExprNode //Constant number
{
public:
    Constant(SymbolEntry *se) : ExprNode(se){dst = new SimpleOperand(se);};
    void output(int level);
    void typeCheck();
    void genCode();
    int ComputeValue();
};

class Id : public ExprNode
{
public:
    Id(SymbolEntry *se) : ExprNode(se){
        SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        dst = new SimpleOperand(temp);};
    void output(int level);
    void typeCheck();
    void genCode();
    int ComputeValue();
};

class ArrayId : public ExprNode
{
private:
    Node *offset;
    // use ArraySize to find
public:
    Node* getArraysize(){return offset;}
    ArrayId(SymbolEntry *se, Node *offset) : ExprNode(se), offset(offset) {
        SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        dst = new SimpleOperand(temp);};
    void output(int level);
    void typeCheck();
    void genCode();
    int ComputeValue();
};

class StmtNode : public Node
{};

class EmptyStmt : public StmtNode
{
public:
    void output(int level);
    void typeCheck();
    void genCode();
};

class CompoundStmt : public StmtNode
{
private:
    StmtNode *stmt;
public:
    CompoundStmt(StmtNode *stmt) : stmt(stmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class SeqNode : public StmtNode
{
private:
    StmtNode *stmt1, *stmt2;
public:
    SeqNode(StmtNode *stmt1, StmtNode *stmt2) : stmt1(stmt1), stmt2(stmt2){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class IfStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
public:
    IfStmt(ExprNode *cond, StmtNode *thenStmt) : cond(cond), thenStmt(thenStmt){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class IfElseStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
    StmtNode *elseStmt;
public:
    IfElseStmt(ExprNode *cond, StmtNode *thenStmt, StmtNode *elseStmt) : cond(cond), thenStmt(thenStmt), elseStmt(elseStmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class WhileStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *doStmt;
    BasicBlock* end_bb;
    BasicBlock* cond_bb;
public:
    WhileStmt(ExprNode *cond, StmtNode *doStmt) : cond(cond), doStmt(doStmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
    BasicBlock* getExt_bb(){
        return end_bb;
    }
    BasicBlock* getCond_bb(){
        return cond_bb;
    }
};

class ForStmt : public StmtNode
{
private:
    StmtNode *iniStmt;
    ExprNode *cond;
    StmtNode *incStmt;
    StmtNode *doStmt;
public:
    ForStmt(StmtNode *iniStmt,ExprNode *cond, StmtNode *incStmt, StmtNode *doStmt) : iniStmt(iniStmt), cond(cond), incStmt(incStmt), doStmt(doStmt) {};
    void typeCheck();
    void genCode();
};

class ReturnStmt : public StmtNode
{
private:
    ExprNode *retValue;
public:
    ReturnStmt(ExprNode*retValue) : retValue(retValue) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class BreakStmt : public StmtNode
{
private:
    WhileStmt* within_while;
public:
    BreakStmt(){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ContinueStmt : public StmtNode
{
private:
    WhileStmt* within_while;
public:
    ContinueStmt() {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ExpStmt : public StmtNode
{
private:
    ExprNode *expr;
public:
    ExpStmt(ExprNode *expr) : expr(expr) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class AssignStmt : public StmtNode
{
private:
    ExprNode *lval;
    ExprNode *expr;
public:
    AssignStmt(ExprNode *lval, ExprNode *expr) : lval(lval), expr(expr) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class InitID : public Node
{
protected:
    SymbolEntry *symbolEntry;
    char *name;
    bool have_init = false;
    Node *array_size;
public:
    InitID(char *name, Node *array_size = nullptr){
        int len = strlen(name);
        this->name = new char[len];
        this->array_size = array_size;
        strcpy(this->name,name);
    };
    ~InitID(){
        delete[]name;
    }
    void output(int level);
    char* getName(){return name;};
    void typeCheck();
    void genCode();
    void SetSymPtr(SymbolEntry *symbolEntry) {this->symbolEntry = symbolEntry;};
    SymbolEntry* getSymPtr() {return symbolEntry;};
    bool have_Init() {return have_init;};
    Node *getArraySize() {return array_size;};
};

class InitAssign : public InitID
{
private:
    ExprNode *val;
public:
    InitAssign(char *name, ExprNode *val) : InitID(name), val(val) {this->have_init = true;};
    void output(int level);
    void typeCheck();
    void genCode();
    ExprNode *getVal() {return val;};
};

class InitArrayAssign : public InitID
{
private:
    Node *init_mtx;
public:
    InitArrayAssign(char *name, Node *size, Node *init_mtx) : InitID(name, size), init_mtx(init_mtx) {this->have_init = true;};
    void output(int level);
    void typeCheck();
    void genCode();
    Node *getMtx() {return init_mtx;};
};

class IDList : public Node
{
private:
    Node *id;
    Node *prev;
public:
    IDList(Node *id) {
        this->id = id;
        this->prev = nullptr;
    };
    IDList(Node *id, Node *prev) {
        this->id = id;
        this->prev = prev;
    };
    void output(int level);
    Node* getId(){return id;};
    Node* getPrev(){return prev;};
    void typeCheck();
    void genCode();
    void arrayInit(Type *itType, Operand *src, Node* mtx_item = nullptr);
};

class FunctionDef : public StmtNode
{
private:
    SymbolEntry *se;
    Node *idList;
    StmtNode *stmt;
public:
    FunctionDef(SymbolEntry *se, Node *idList, StmtNode *stmt) : se(se), idList(idList), stmt(stmt){};
    FunctionDef(SymbolEntry *se, StmtNode *stmt) : se(se), idList(nullptr), stmt(stmt){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ParamList : public Node
{
private:
    Node *val;
    Node *prev;
public:
    ParamList(Node *val) {
        this->val = val;
        this->prev = nullptr;
    };
    ParamList(Node *val, Node *prev) {
        this->val = val;
        this->prev = prev;
    };
    void output(int level);
    Node *getValue() {return val;};
    Node *getPrev() {return prev;};
    Operand *getOperand() {return ((ExprNode *)val)->getOperand();};
    SymbolEntry *getSymPtr() {return ((ExprNode *)val)->getSymPtr();};
    void typeCheck();
    void genCode();
};

class FuncCall : public ExprNode
{
private:
    SymbolEntry *FuncSe;
    Node* params;
public:
    FuncCall(SymbolEntry *se, SymbolEntry *FuncSe, Node *params) : ExprNode(se), FuncSe(FuncSe), params(params) {
        SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        dst = new SimpleOperand(temp);};
    FuncCall(SymbolEntry *se, SymbolEntry *FuncSe) : ExprNode(se), FuncSe(FuncSe), params(nullptr) {
        SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        dst = new SimpleOperand(temp);};
    void output(int level);
    void typeCheck();
    void genCode();
    int ComputeValue();
};

class SysFuncCall : public ExprNode
{
private:
    Node* params;
    int kind;
public:
    enum{PUTINT, GETINT, PUTCH, GETCH};
    SysFuncCall(SymbolEntry *se, Node *params, int kind = -1) : ExprNode(se), params(params) {
        this->kind = kind;
        SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        dst = new SimpleOperand(temp);
        };
    SysFuncCall(SymbolEntry *se, int kind = -1) : ExprNode(se), params(nullptr) {
        this->kind = kind;
        SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        dst = new SimpleOperand(temp);
        };
    void output(int level);
    void typeCheck();
    void genCode();
    int getKind(){return kind;};
    int ComputeValue();
};

class DeclStmt : public StmtNode
{
private:
    Node *idList;
public:
    DeclStmt(Node *idList) : idList(idList){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ConstDeclStmt : public StmtNode
{
private:
    Node *idList;
public:
    ConstDeclStmt(Node *idList) : idList(idList){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ArraySize : public Node
{
private:
    ExprNode *size;
    Node *prev;
    int ComputedSize = 0;
public:
    //传递oprand
    Operand *last_op;
    //传递Type
    Type* arr_type;
    Operand *dst;
    ArraySize(ExprNode *size, Node *prev = nullptr) : size(size), prev(prev){};
    ExprNode *getSize() {return size;};
    Node *getPrev() {return prev;};
    void output(int level);
    void typeCheck();
    void genCode();
    int getCpdSize(){return ComputedSize;}
};

class ArrayInit : public Node
{
private:
    Node *list;
    Node *next;
public:
    ArrayInit(Node *list, Node *next = nullptr) : list(list), next(next){};
    Node *getList() {return list;};
    Node *getNext() {return next;};
    void output(int level);
    void typeCheck();
    void genCode();
};

class Ast
{
private:
    Node* root;
public:
    Ast() {root = nullptr;}
    void setRoot(Node*n) {root = n;}
    void output();
    void typeCheck();
    void genCode(Unit *unit);
};

#endif
