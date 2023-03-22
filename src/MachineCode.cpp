#include "MachineCode.h"
#include<iostream>
#include"Operand.h"
#include"Type.h"
extern FILE* yyout;

MachineOperand::MachineOperand(int tp, int val)
{
    this->type = tp;
    if(tp == MachineOperand::IMM)
        this->val = val;
    else 
        this->reg_no = val;
}

MachineOperand::MachineOperand(std::string label)
{
    this->type = MachineOperand::LABEL;
    this->label = label;
}

bool MachineOperand::operator==(const MachineOperand&a) const
{
    if (this->type != a.type)
        return false;
    if (this->type == IMM)
        return this->val == a.val;
    return this->reg_no == a.reg_no;
}

bool MachineOperand::operator<(const MachineOperand&a) const
{
    if(this->type == a.type)
    {
        if(this->type == IMM)
            return this->val < a.val;
        return this->reg_no < a.reg_no;
    }
    return this->type < a.type;

    if (this->type != a.type)
        return false;
    if (this->type == IMM)
        return this->val == a.val;
    return this->reg_no == a.reg_no;
}

void MachineOperand::PrintReg()
{
    switch (reg_no)
    {
    case 11:
        fprintf(yyout, "fp");
        break;
    case 13:
        fprintf(yyout, "sp");
        break;
    case 14:
        fprintf(yyout, "lr");
        break;
    case 15:
        fprintf(yyout, "pc");
        break;
    default:
        fprintf(yyout, "r%d", reg_no);
        break;
    }
}

void MachineOperand::output() 
{
    /* HINT：print operand
    * Example:
    * immediate num 1 -> print #1;
    * register 1 -> print r1;
    * lable addr_a -> print addr_a; */
    switch (this->type)
    {
    case IMM:
        fprintf(yyout, "#%d", this->val);
        break;
    case VREG:
        fprintf(yyout, "v%d", this->reg_no);
        break;
    case REG:
        PrintReg();
        break;
    case LABEL:
        if(this->label.substr(0,5) == ".func"){
            std::string cut = label.substr(5,label.length()); 
            fprintf(yyout, "%s", cut.c_str());
        }
        else if (this->label.substr(0, 2) == ".L")
            fprintf(yyout, "%s", this->label.c_str());
        else
            fprintf(yyout, "addr_%s", this->label.c_str());
    default:
        break;
    }
}

void MachineInstruction::PrintCond()
{
    switch (cond)
    {
    case LT:
        fprintf(yyout, "lt");
        break;
    case EQ:
        fprintf(yyout, "eq");
        break;
    case NE:
        fprintf(yyout, "ne");
        break;
    case LE:
        fprintf(yyout, "le");
        break;
    case GT:
        fprintf(yyout, "gt");
        break;
    case GE:
        fprintf(yyout, "ge");
        break;
    default:
        break;
    }
}

BinaryMInstruction::BinaryMInstruction(
    MachineBlock* p, int op, 
    MachineOperand* dst, MachineOperand* src1, MachineOperand* src2, 
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::BINARY;
    this->op = op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    dst->setParent(this);
    src1->setParent(this);
    src2->setParent(this);
}

void BinaryMInstruction::output() 
{
    // TODO: 
    // Complete other instructions
    switch (this->op)
    {
    case BinaryMInstruction::ADD:
        fprintf(yyout, "\tadd ");
        this->PrintCond();
        this->def_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[1]->output();
        fprintf(yyout, "\n");
        break;
    case BinaryMInstruction::SUB:
        fprintf(yyout, "\tsubs ");
        this->PrintCond();
        this->def_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[1]->output();
        fprintf(yyout, "\n");
        break;
    case BinaryMInstruction::MULT:
        fprintf(yyout, "\tmul ");
        this->PrintCond();
        this->def_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[1]->output();
        fprintf(yyout, "\n");
        break;
    
    case BinaryMInstruction::AND:
        fprintf(yyout, "\tands ");
        this->PrintCond();
        this->def_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[1]->output();
        fprintf(yyout, "\n");
        break;
    case BinaryMInstruction::XOR:
        fprintf(yyout, "\teors ");
        this->PrintCond();
        this->def_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[1]->output();
        fprintf(yyout, "\n");
        break;
    case BinaryMInstruction::OR:
        fprintf(yyout, "\torrs ");
        this->PrintCond();
        this->def_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[1]->output();
        fprintf(yyout, "\n");
        break;
    case BinaryMInstruction::DIV:
        fprintf(yyout, "\tsdiv ");
        this->PrintCond();
        this->def_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[1]->output();
        fprintf(yyout, "\n");
        break;
    case BinaryMInstruction::MOD:
        fprintf(yyout, "\torrs ");
        this->PrintCond();
        this->def_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[0]->output();
        fprintf(yyout, ", ");
        this->use_list[1]->output();
        fprintf(yyout, "\n");
        break;
    case BinaryMInstruction::DUMMY:
        break;
    default:
        break;
    }
}

LoadMInstruction::LoadMInstruction(MachineBlock* p,
    MachineOperand* dst, MachineOperand* src1, MachineOperand* src2,
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::LOAD;
    this->op = -1;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src1);
    if (src2)
        this->use_list.push_back(src2);
    dst->setParent(this);
    src1->setParent(this);
    if (src2)
        src2->setParent(this);
}

void LoadMInstruction::output()
{
    fprintf(yyout, "\tldr ");
    this->def_list[0]->output();
    fprintf(yyout, ", ");

    // Load immediate num, eg: ldr r1, =8
    if(this->use_list[0]->isImm())
    {
        fprintf(yyout, "=%d\n", this->use_list[0]->getVal());
        return;
    }

    if(this->use_list[0]->isLabel())
    {
        fprintf(yyout, "=");
        this->use_list[0]->output();
        fprintf(yyout, "\n");
        return;
    }

    // Load address
    if(this->use_list[0]->isReg()||this->use_list[0]->isVReg())
        fprintf(yyout, "[");

    this->use_list[0]->output();
    if( this->use_list.size() > 1 )
    {
        fprintf(yyout, ", ");
        this->use_list[1]->output();
    }

    if(this->use_list[0]->isReg()||this->use_list[0]->isVReg())
        fprintf(yyout, "]");
    fprintf(yyout, "\n");
}

StoreMInstruction::StoreMInstruction(MachineBlock* p,
    MachineOperand* src, MachineOperand* dst_base, MachineOperand* dst_offset, 
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::STORE;
    this->op = -1;
    this->cond = cond;
    this->use_list.push_back(src);
    if (dst_base)
        this->use_list.push_back(dst_base);
    if (dst_offset)
        this->use_list.push_back(dst_offset);
    src->setParent(this);
    if (dst_base)
        dst_base->setParent(this);
    if (dst_offset)
        dst_offset->setParent(this);

}

void StoreMInstruction::output()
{
    fprintf(yyout, "\tstr ");
    this->use_list[0]->output();
    fprintf(yyout, ", ");
    if(this->use_list[1]->isImm())
    {
        fprintf(yyout, "=%d\n", this->use_list[1]->getVal());
        return;
    }

    if(this->use_list[1]->isLabel())
    {
        fprintf(yyout, "[");
        this->use_list[1]->output();
        fprintf(yyout, "]");
        fprintf(yyout, "\n");
        return;
    }

    if(this->use_list[1]->isReg()||this->use_list[1]->isVReg())
        fprintf(yyout, "[");

    this->use_list[1]->output();
    if( this->use_list.size() > 2 )
    {
        fprintf(yyout, ", ");
        this->use_list[2]->output();
    }

    if(this->use_list[1]->isReg()||this->use_list[1]->isVReg())
        fprintf(yyout, "]");
    fprintf(yyout, "\n");
}

MovMInstruction::MovMInstruction(MachineBlock* p, int op, 
    MachineOperand* dst, MachineOperand* src,
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::MOV;
    this->op = op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src);
    src->setParent(this);
    dst->setParent(this);
}

void MovMInstruction::output() 
{
    switch (op)
    {
    case MOV:{
        fprintf(yyout, "\tmov");
        if (cond != MachineInstruction::NONE) {
            switch(cond) {
            case (MachineInstruction::EQ):
                fprintf(yyout, "eq ");
                break;
            case (MachineInstruction::NE):
                fprintf(yyout, "ne ");
                break;
            case (MachineInstruction::LT):
                fprintf(yyout, "lt ");
                break;
            case (MachineInstruction::LE):
                fprintf(yyout, "le ");
                break;
            case (MachineInstruction::GT):
                fprintf(yyout, "gt ");
                break;
            case (MachineInstruction::GE):
                fprintf(yyout, "ge ");
                break;
            }
        }
        else {
            fprintf(yyout, " ");
        }
        if(def_list[0]->isImm()) {fprintf(yyout, "w");}
        def_list[0]->output();
        fprintf(yyout, ", ");
        if(use_list[0]->isLabel())
            fprintf(yyout, "#");
        use_list[0]->output();
        fprintf(yyout, "\n");
        /* code */
        break;
    }
    case MVN:{
        fprintf(yyout, "\tmvn ");
        def_list[0]->output();
        fprintf(yyout, ", ");
        MachineOperand* temp_imm = new MachineOperand(MachineOperand::IMM, (-1 - use_list[0]->getVal()));
        if(temp_imm->isImm()) fprintf(yyout, "");
        temp_imm->output();
        fprintf(yyout, "\n");
        break;
    }
    default:
        break;
    }
}

BranchMInstruction::BranchMInstruction(MachineBlock* p, int op, 
    MachineOperand* dst, 
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::BRANCH;
    this->op = op;
    this->use_list.push_back(dst);
    this->cond = cond;
    if(op == BranchMInstruction::B) this->isUncondBranch = true;
}

void BranchMInstruction::output()
{
    // Not Finished Yet~
    MachineOperand *dst = use_list[0];
    switch(op){
    case B:
        fprintf(yyout, "\tb");
        PrintCond();
        fprintf(yyout, " ");
        dst->output();
        fprintf(yyout, "\n");
        break;
    case BL:
        fprintf(yyout, "\tbl ");
        dst->output();
        fprintf(yyout, "\n");
        break;
    case BX:
        fprintf(yyout, "\tbx ");
        dst->output();
        fprintf(yyout, "\n");
        break;
    default:
        fprintf(yyout, "\tWRONG BRANCH INSTRUCTION OPCODE!");
    }
}

CmpMInstruction::CmpMInstruction(MachineBlock* p, 
    MachineOperand* src1, MachineOperand* src2, 
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::CMP;
    this->op = -1;
    this->cond = cond;
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    src1->setParent(this);
    src2->setParent(this);
}

void CmpMInstruction::output()
{
    fprintf(yyout, "\tcmp ");
    //this->PrintCond();
    this->use_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[1]->output();
    fprintf(yyout, "\n");
}

StackMInstrcution::StackMInstrcution(MachineBlock* p, int op, 
    MachineOperand* src, MachineOperand* src2,
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::STACK;
    this->op = op;
    this->cond = cond;
    this->use_list.push_back(src);
    if (src2 != nullptr) {
        this->use_list.push_back(src2);
    }
    src->setParent(this);
}

void StackMInstrcution::output()
{
    if (op == PUSH) {
        fprintf(yyout, "\tpush {");
        this->use_list[0]->output();
        if (this->use_list.size() > 1) {
            fprintf(yyout, ", ");
            this->use_list[1]->output();
        }
        fprintf(yyout, "}\n");
    }
    else if (op == POP) {
        fprintf(yyout, "\tpop {");
        this->use_list[0]->output();
        if (this->use_list.size() > 1) {
            fprintf(yyout, ", ");
            this->use_list[1]->output();
        }
        fprintf(yyout, "}\n");
    }
    
}

GlobalMInstrcution::GlobalMInstrcution(MachineBlock* p, 
                MachineOperand* dst, MachineOperand* src, 
                int cond)
{
    this->parent = p;
    this->type = MachineInstruction::GLOBAL;
    this->op = -1;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src);
    
    dst->setParent(this);
    src->setParent(this);
}

void GlobalMInstrcution::output()
{
    //fprintf(yyout, "\t.align 4\n");
    //fprintf(yyout, "\t.global ");
    //def_list[0]->output();
    //fprintf(yyout, "\n");
    //fprintf(yyout, "\t.size ");
    //def_list[0]->output();
    //fprintf(yyout, ", 4\n");
    def_list[0]->output();
    fprintf(yyout, ":\n");
    fprintf(yyout, "\t.word ");
    fprintf(yyout, "%d", use_list[0]->getVal());
    fprintf(yyout, "\n");
}

ArrayGlobalMInstrcution::ArrayGlobalMInstrcution(MachineBlock* p, 
                MachineOperand* dst, ArrayType* tp,
                int cond)
{
    this->parent = p;
    this->tp = tp;
    this->type = MachineInstruction::GLOBAL;
    this->op = -1;
    this->cond = cond;
    this->def_list.push_back(dst);
    
    dst->setParent(this);
}

void ArrayGlobalMInstrcution::output()
{
    int space = 4;
    ArrayType* arr_tp = tp;
    while(arr_tp->isArray()){
        space *= arr_tp->getItemCount();
        arr_tp = (ArrayType*)(arr_tp->getItemType());
    }
    //fprintf(yyout, "\t.align 4\n");
    //fprintf(yyout, "\t.global ");
    //def_list[0]->output();
    //fprintf(yyout, "\n");
    //fprintf(yyout, "\t.size ");
    //def_list[0]->output();
    //fprintf(yyout, ", 4\n");
    def_list[0]->output();
    fprintf(yyout, ":\n");
    fprintf(yyout, "\t.space ");
    fprintf(yyout, "%d", space);
    fprintf(yyout, "\n");
}

MachineFunction::MachineFunction(MachineUnit* p, SymbolEntry* sym_ptr, std::vector<MachineOperand*>& params, bool ret_void) 
{ 
    this->parent = p; 
    this->sym_ptr = sym_ptr; 
    this->stack_size = 0;
    this->is_leaf = true;
    this->ret_void = ret_void; // set false as default
    this->params = params;
};

void MachineBlock::output()
{
    fprintf(yyout, ".L%d:\n", this->no);
    for(auto iter : inst_list) {
        iter->output();
        if(iter->isUncondBranch && iter->isUnCond()) break;
    }
}
void MachineBlock::InsertAfter(MachineInstruction* inst, MachineInstruction* inst_to){

    std::vector<MachineInstruction*> ::iterator t;
    t = find(inst_list.begin(),inst_list.end(),inst_to);
    if(t == inst_list.end()){return;}
    inst_list.insert(t + 1, inst);
}
void MachineFunction::finishFuncHead()
{
    int regn = 0;
    for(auto param : params) {
        if (regn < 4) {
            auto inst = new MovMInstruction(block_list[0], MovMInstruction::MOV,
                param, new MachineOperand(MachineOperand::REG, regn));
            block_list[0]->InsertBefore(inst);
        }
        else {
            int off = saved_regs.size();
            if (is_leaf) {
                off += 1;
            }
            else {
                off += 2;
            }
            off += regn - 4;
            off *= 4;
            auto inst = new LoadMInstruction(block_list[0], param, new MachineOperand(MachineOperand::REG, 7), 
                new MachineOperand(MachineOperand::IMM, off));
            block_list[0]->InsertBefore(inst);
        }
        regn++;
    }
}

void MachineFunction::output()
{
    //const char *func_name = this->sym_ptr->toStr().c_str() + 1;
    fprintf(yyout, "\t.global %s\n", this->sym_ptr->toStr().c_str());
    fprintf(yyout, "\t.type %s , %%function\n", this->sym_ptr->toStr().c_str());
    fprintf(yyout, "%s:\n", this->sym_ptr->toStr().c_str());
    // TODO
    /* Hint:
    *  1. Save fp
    *  2. fp = sp
    *  3. Save callee saved register
    *  4. Allocate stack space for local variable */
    if (is_leaf) {
        fprintf(yyout, "\tpush {fp}\n");
    }
    else {
        fprintf(yyout, "\tpush {fp, lr}\n");
    }
    if (!(saved_regs.empty())){
        std::set<int>::iterator it = saved_regs.begin();
        fprintf(yyout, "\tpush {r%d", *it);
        it++;
        for (; it != saved_regs.end(); it++){
            fprintf(yyout, ", r%d", *it);
        }
        fprintf(yyout, "}\n");
    }
    fprintf(yyout, "\tadd fp, sp, #0\n");
    fprintf(yyout, "\tsub sp, sp, #%d\n", stack_size);
    
    // Traverse all the block in block_list to print assembly code.

    for(auto retins : ret_inst) {
        MachineBlock *mblock = retins->getParent();
        MachineInstruction *inst;
        std::vector<MachineInstruction*>::iterator it = (mblock->getInsts()).begin();
        for (; it != (mblock->getInsts()).end(); it++) {
            if (*it == retins) {
                if (is_leaf) {
                    inst = new StackMInstrcution(mblock, StackMInstrcution::POP,
                        new MachineOperand(MachineOperand::REG, 11));
                    it = (mblock->getInsts()).insert(it, inst);
                }
                else {
                    it = (mblock->getInsts()).erase(it);
                    inst = new StackMInstrcution(mblock, StackMInstrcution::POP,
                        new MachineOperand(MachineOperand::REG, 11),
                        new MachineOperand(MachineOperand::REG, 15));
                    it = (mblock->getInsts()).insert(it, inst);
                }
                
                std::set<int>::reverse_iterator sit = saved_regs.rbegin();
                std::set<int>::reverse_iterator stop = saved_regs.rend();
                for (; sit != stop; sit++){
                    inst = new StackMInstrcution(mblock, StackMInstrcution::POP,
                        new MachineOperand(MachineOperand::REG, *sit));
                    it = (mblock->getInsts()).insert(it, inst);
                }
                break;
            }
        }
    }

    for(auto iter : block_list)
        iter->output();
    
    if (ret_void) {
        fprintf(yyout, "\tadd sp, fp, #0\n");

        if (saved_regs.size() != 0) {
            MachineInstruction *inst;
            std::set<int>::iterator sit = saved_regs.begin();
            std::set<int>::iterator stop = saved_regs.end();
            fprintf(yyout, "\tpop {r%d", *sit);
            sit++;
            for (; sit != stop; sit++){
                fprintf(yyout, ", r%d", *sit);
            }
            fprintf(yyout, "}\n");
        }
        

        if (is_leaf) {
        fprintf(yyout, "\tpop {fp}\n");
        }
        else {
            fprintf(yyout, "\tpop {fp, pc}\n");
        }
    }
}

void MachineUnit::PrintGlobalDecl()
{
    bool title = true;
    for(auto iter : global_list){
        if(title) {
            fprintf(yyout, "\t.data\n");
            title = false;
        }
        iter->output();
    }
}

void MachineUnit::output()
{
    // TODO
    /* Hint:
    * 1. You need to print global variable/const declarition code;
    * 2. Traverse all the function in func_list to print assembly code;
    * 3. Don't forget print bridge label at the end of assembly code!! */
    fprintf(yyout, "\t.arch armv8-a\n");
    fprintf(yyout, "\t.arch_extension crc\n");
    fprintf(yyout, "\t.arm\n");
    
    PrintGlobalDecl();
    
    fprintf(yyout, "\t.text\n");
    
    fprintf(yyout, "\t.global __aeabi_idiv\n");
    //fprintf(yyout, "\t.global putint\n");
    //fprintf(yyout, "\t.global putch\n");
    //fprintf(yyout, "\t.global getint\n");
    for(auto iter : func_list)
        iter->output();
}
