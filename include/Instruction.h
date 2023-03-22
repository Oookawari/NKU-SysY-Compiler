#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

#include "Operand.h"
#include "AsmBuilder.h"
#include <vector>
#include <map>
#include <sstream>
#include "Ast.h"
#include "Type.h"
class BasicBlock;

class Instruction
{
public:
    Instruction(unsigned instType, BasicBlock *insert_bb = nullptr);
    virtual ~Instruction();
    BasicBlock *getParent();
    bool isUncond() const {return instType == UNCOND;};
    bool isCond() const {return instType == COND;};
    bool isAlloc() const {return instType == ALLOCA;};
    void setParent(BasicBlock *);
    void setNext(Instruction *);
    void setPrev(Instruction *);
    Instruction *getNext();
    Instruction *getPrev();
    bool isRet() {return instType == RET;};
    virtual void output() const = 0;
    MachineOperand* genMachineOperand(Operand*);
    MachineOperand* genMachineReg(int reg);
    MachineOperand* genMachineVReg();
    MachineOperand* genMachineImm(int val);
    MachineOperand* genMachineLabel(int block_no);
    virtual void genMachineCode(AsmBuilder*) = 0;
protected:
    unsigned instType;
    unsigned opcode;
    Instruction *prev;
    Instruction *next;
    BasicBlock *parent;
    std::vector<Operand*> operands;
    enum {BINARY, UNARY, COND, UNCOND, RET, LOAD, STORE, CMP, ALLOCA, GLOBAL_INIT, CALL, PUTINT_CALL, GETINT_CALL, ZEXT ,GETPTR}; 
};

// meaningless instruction, used as the head node of the instruction list.
class DummyInstruction : public Instruction
{
public:
    DummyInstruction() : Instruction(-1, nullptr) {};
    void output() const {};
    void genMachineCode(AsmBuilder*) {};
};

class AllocaInstruction : public Instruction
{
    int align = 4;
public:
    AllocaInstruction(Operand *dst, SymbolEntry *se, BasicBlock *insert_bb = nullptr);
    //AllocaInstruction(Operand *dst, SymbolEntry *se, int align, BasicBlock *insert_bb = nullptr);
    ~AllocaInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
private:
    SymbolEntry *se;
};

class LoadInstruction : public Instruction
{
public:
    LoadInstruction(Operand *dst, Operand *src_addr, BasicBlock *insert_bb = nullptr);
    ~LoadInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};

class StoreInstruction : public Instruction
{
public:
    StoreInstruction(Operand *dst_addr, Operand *src, BasicBlock *insert_bb = nullptr);
    ~StoreInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};

class BinaryInstruction : public Instruction
{
public:
    BinaryInstruction(unsigned opcode, Operand *dst, Operand *src1, Operand *src2, BasicBlock *insert_bb = nullptr);
    ~BinaryInstruction();
    void output() const;
    enum {ADD, SUB, MULT, DIV, MOD, AND, XOR, OR, LOR, LAND};
    void genMachineCode(AsmBuilder*);
};

class CmpInstruction : public Instruction
{
public:
    CmpInstruction(unsigned opcode, Operand *dst, Operand *src1, Operand *src2, BasicBlock *insert_bb = nullptr);
    ~CmpInstruction();
    void output() const;
    enum {E, NE, L, GE, G, LE};
    void genMachineCode(AsmBuilder*);
};

// unconditional branch
class UncondBrInstruction : public Instruction
{
public:
    UncondBrInstruction(BasicBlock*, BasicBlock *insert_bb = nullptr);
    void output() const;
    void setBranch(BasicBlock *);
    BasicBlock *getBranch();
    void genMachineCode(AsmBuilder*);
protected:
    BasicBlock *branch;
};

// conditional branch
class CondBrInstruction : public Instruction
{
public:
    CondBrInstruction(BasicBlock*, BasicBlock*, Operand *, BasicBlock *insert_bb = nullptr);
    ~CondBrInstruction();
    void output() const;
    void setTrueBranch(BasicBlock*);
    BasicBlock* getTrueBranch();
    void setFalseBranch(BasicBlock*);
    BasicBlock* getFalseBranch();
    void genMachineCode(AsmBuilder*);
protected:
    BasicBlock* true_branch;
    BasicBlock* false_branch;
};

class RetInstruction : public Instruction
{
public:
    RetInstruction(Operand *src, BasicBlock *insert_bb = nullptr);
    ~RetInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};
/*
class TruncInstruction : public Instruction
{
private:
    char* dst_size;
public:
    TruncInstruction(Operand *dst, Operand *src, char *dst_size, BasicBlock *insert_bb = nullptr);
    ~TruncInstruction();
    void output() const;
};*/

class GlobalInitInstruction : public Instruction
{
    int align;
    bool is_array;
public:
    GlobalInitInstruction(Operand *dst_addr, Operand *src, BasicBlock *insert_bb = nullptr);
    ~GlobalInitInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};

class CallInstruction : public Instruction
{
private:
    std::vector<ExprNode*> params;
    std::vector<Operand*> ops;
    SymbolEntry *func_se;
public:
    CallInstruction(Operand *dst, SymbolEntry *func_se, std::vector<ExprNode*> params, std::vector<Operand*> ops, BasicBlock *insert_bb);
    ~CallInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};

class PutInstruction : public Instruction
{
    private:
    int kind;
public:
    enum{PUTINT,PUTCH};
    PutInstruction(Operand *src, BasicBlock *insert_bb, int kind);
    ~PutInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};

class GetIntInstruction : public Instruction
{
public:
    GetIntInstruction(Operand *dst, BasicBlock *insert_bb);
    ~GetIntInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};

class GetChInstruction : public Instruction
{
public:
    GetChInstruction(Operand *dst, BasicBlock *insert_bb);
    ~GetChInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};

class ZextInstruction : public Instruction
{
public:
    ZextInstruction(Operand *dst, Operand *src, BasicBlock *insert_bb = nullptr);
    ~ZextInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
};

class GetPtrInstruction : public Instruction
{
private:
    Type* ptr_type;
public:
    bool isGlobal = false;
    GetPtrInstruction(Operand *dst, Operand *src, Operand* offset, Type* ptr, BasicBlock *insert_bb = nullptr) ;
    ~GetPtrInstruction();
    void output() const;
    void genMachineCode(AsmBuilder*);
    Type* getType(){return ptr_type;}
};
#endif