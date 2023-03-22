#ifndef __ASMBUILDER_H__
#define __ASMBUILDER_H__

#include "MachineCode.h"
#include <map>
#include "SymbolTable.h"
class Cond_inst
{
    MachineInstruction* cond_inst;
    int cond_opcode;
public:
    Cond_inst(){};
    Cond_inst(const Cond_inst& copy) {
        this->cond_inst = copy.cond_inst;
        this->cond_opcode = copy.cond_opcode;
    };
    Cond_inst(MachineInstruction* cond_inst, int cond_opcode) { 
        this->cond_inst = cond_inst;
        this->cond_opcode = cond_opcode;
    };
    MachineInstruction* getInstruction(){return cond_inst;}
    int getOpCode(){return cond_opcode;}
};

class AsmBuilder
{
private:
    MachineUnit* mUnit;  // mahicne unit
    MachineFunction* mFunction; // current machine code function;
    MachineBlock* mBlock; // current machine code block;
    int cmpOpcode; // CmpInstruction opcode, for CondInstruction;
public:
    std::map<SymbolEntry*, Cond_inst> op_cond_map;
    void setUnit(MachineUnit* unit) { this->mUnit = unit; };
    void setFunction(MachineFunction* func) { this->mFunction = func; };
    void setBlock(MachineBlock* block) { this->mBlock = block; };
    void setCmpOpcode(int opcode) { this->cmpOpcode = opcode; };
    MachineUnit* getUnit() { return this->mUnit; };
    MachineFunction* getFunction() { return this->mFunction; };
    MachineBlock* getBlock() { return this->mBlock; };
    int getCmpOpcode() { return this->cmpOpcode; };
    void insert_map(SymbolEntry* se, int cond_op, MachineInstruction* cond_inst);
    Cond_inst search(SymbolEntry* se);
};


#endif