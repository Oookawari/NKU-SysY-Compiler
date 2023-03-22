#include "Function.h"
#include "Unit.h"
#include "Type.h"
#include <list>

extern FILE* yyout;

Function::Function(Unit *u, SymbolEntry *s)
{
    u->insertFunc(this);
    entry = new BasicBlock(this);
    sym_ptr = s;
    parent = u;
}

Function::~Function()
{
    // auto delete_list = block_list;
    // for (auto &i : delete_list)
    //     delete i;
    // parent->removeFunc(this);
}

// remove the basicblock bb from its block_list.
void Function::remove(BasicBlock *bb)
{
    block_list.erase(std::find(block_list.begin(), block_list.end(), bb));
}

void Function::output() const
{
    FunctionType* funcType = dynamic_cast<FunctionType*>(sym_ptr->getType());
    Type *retType = funcType->getRetType();
    std::vector<SymbolEntry*> paramsSe = funcType->getParamsSe();
    
    if (paramsSe.size() == 0)
        fprintf(yyout, "define %s %s() {\n", retType->toStr().c_str(), sym_ptr->toStr().c_str());
    else {
        fprintf(yyout, "define %s %s(", retType->toStr().c_str(), sym_ptr->toStr().c_str());
        for (long unsigned int i = 0; i < paramsSe.size() - 1; i++) {
            fprintf(yyout, "%s %s, ", paramsSe[i]->getType()->toStr().c_str(),
                    paramsSe[i]->toStr().c_str());
        }
        fprintf(yyout, "%s %s) {\n", paramsSe[paramsSe.size() - 1]->getType()->toStr().c_str(),
                    paramsSe[paramsSe.size() - 1]->toStr().c_str());
    }
    // fprintf(yyout, "block number: %d", block_list.size());

    std::set<BasicBlock *> v;
    std::list<BasicBlock *> q;
    q.push_back(entry);
    v.insert(entry);
    while (!q.empty())
    {
        auto bb = q.front();
        q.pop_front();
        bb->output();
        for (auto succ = bb->succ_begin(); succ != bb->succ_end(); succ++)
        {
            if (v.find(*succ) == v.end())
            {
                v.insert(*succ);
                q.push_back(*succ);
            }
        }
    }
    fprintf(yyout, "}\n");
}

MachineOperand *Function::genMachineOperand(Operand *ope)
{
    MachineOperand* mope = nullptr;
    if(ope->IsTrue()){
        mope = new MachineOperand(MachineOperand::IMM, 1);
    }
    else if(ope->IsZero()){
        mope = new MachineOperand(MachineOperand::IMM, 0);
    }
    else if(ope->IsSimple()){
        auto se = ((SimpleOperand*)ope)->getEntry();
    
        if(se->isConstant())
            mope = new MachineOperand(MachineOperand::IMM, dynamic_cast<ConstantSymbolEntry*>(se)->getValue());
        else if(se->isTemporary())
            mope = new MachineOperand(MachineOperand::VREG, dynamic_cast<TemporarySymbolEntry*>(se)->getLabel());
        else if(se->isVariable())
        {
            auto id_se = dynamic_cast<IdentifierSymbolEntry*>(se);
            if(id_se->isGlobal())
                mope = new MachineOperand(id_se->toStr().c_str());
            else
                exit(0);
        }
    }
    else if(ope->IsInit()) {
        mope = new MachineOperand(MachineOperand::IMM, 0);
    }
    else {
        std::cout <<"操作数没有正确的Kind!" << std::endl;
        exit(0);
    }
    return mope;
}

void Function::genMachineCode(AsmBuilder *builder)
{
    FunctionType* funcType = dynamic_cast<FunctionType*>(sym_ptr->getType());
    std::vector<SymbolEntry*> paramsSe = funcType->getParamsSe();
    std::vector<MachineOperand*> params;
    std::vector<SymbolEntry*>::iterator it = paramsSe.begin();
    bool ret_void = funcType->getRetType()->isVoid();
    
    for (; it != paramsSe.end(); it++) {
        auto pop = genMachineOperand(((IdentifierSymbolEntry*)(*it))->getParamAddr());
        params.push_back(pop);
    }

    auto cur_unit = builder->getUnit();
    auto cur_func = new MachineFunction(cur_unit, this->sym_ptr, params, ret_void);
    builder->setFunction(cur_func);
    std::map<BasicBlock*, MachineBlock*> map;
    for(auto block : block_list)
    {
        block->genMachineCode(builder);
        map[block] = builder->getBlock();
    }
    // Add pred and succ for every block
    for(auto block : block_list)
    {
        auto mblock = map[block];
        for (auto pred = block->pred_begin(); pred != block->pred_end(); pred++)
            mblock->addPred(map[*pred]);
        for (auto succ = block->succ_begin(); succ != block->succ_end(); succ++)
            mblock->addSucc(map[*succ]);
    }
    cur_func->finishFuncHead();
    cur_unit->InsertFunc(cur_func);
}