#include "AsmBuilder.h"
#include <iostream>
void AsmBuilder::insert_map(SymbolEntry *se, int cond_op, MachineInstruction *cond_inst)
{
    op_cond_map[se] = Cond_inst(cond_inst, cond_op);
}

Cond_inst AsmBuilder::search(SymbolEntry* se)
{
    int exist = this->op_cond_map.count(se);
    if(exist == 0) {return Cond_inst(nullptr, MachineInstruction::NONE);}
    return op_cond_map[se];
};