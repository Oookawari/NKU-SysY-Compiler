#include "Instruction.h"
#include "BasicBlock.h"
#include <iostream>
#include "Function.h"
#include "Type.h"
#include "Unit.h"
extern FILE* yyout;

Instruction::Instruction(unsigned instType, BasicBlock *insert_bb)
{
    prev = next = this;
    opcode = -1;
    this->instType = instType;
    if (insert_bb != nullptr)
    {
        insert_bb->insertBack(this);
        parent = insert_bb;
    } 
    if(unit_ptr->global_collecting){
        unit_ptr->insert_init_Follow(this);
    }
}

Instruction::~Instruction()
{
    parent->remove(this);
}

BasicBlock *Instruction::getParent()
{
    return parent;
}

void Instruction::setParent(BasicBlock *bb)
{
    parent = bb;
}

void Instruction::setNext(Instruction *inst)
{
    next = inst;
}

void Instruction::setPrev(Instruction *inst)
{
    prev = inst;
}

Instruction *Instruction::getNext()
{
    return next;
}

Instruction *Instruction::getPrev()
{
    return prev;
}

MachineOperand* Instruction::genMachineOperand(Operand* ope)
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
    else if(ope->IsInit()){
        mope = new MachineOperand(MachineOperand::IMM, 0);
    }
    else {
        std::cout <<"操作数没有正确的Kind!" << std::endl;
        exit(0);
    }
    return mope;
}

MachineOperand* Instruction::genMachineReg(int reg) 
{
    return new MachineOperand(MachineOperand::REG, reg);
}

MachineOperand* Instruction::genMachineVReg() 
{
    return new MachineOperand(MachineOperand::VREG, SymbolTable::getLabel());
}

MachineOperand* Instruction::genMachineImm(int val) 
{
    return new MachineOperand(MachineOperand::IMM, val);
}

MachineOperand* Instruction::genMachineLabel(int block_no)
{
    std::ostringstream buf;
    buf << ".L" << block_no;
    std::string label = buf.str();
    return new MachineOperand(label);
}

BinaryInstruction::BinaryInstruction(unsigned opcode, Operand *dst, Operand *src1, Operand *src2, BasicBlock *insert_bb) : Instruction(BINARY, insert_bb)
{

    this->opcode = opcode;
    operands.push_back(dst);
    if(src1->getType()->isPTR()) {
        TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        Operand *new_dst = new SimpleOperand(temp_se);
        insert_bb->insertBefore(new LoadInstruction(new_dst, src1), this);
        src1 = new_dst;
        
    }
    if(src2->getType()->isPTR()) {
        TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        Operand *new_dst = new SimpleOperand(temp_se);
        insert_bb->insertBefore(new LoadInstruction(new_dst, src2), this);
        src2 = new_dst;
    }
    operands.push_back(src1);
    operands.push_back(src2);
    dst->setDef(this);
    src1->addUse(this);
    src2->addUse(this);
}

BinaryInstruction::~BinaryInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
    operands[2]->removeUse(this);
}

void BinaryInstruction::output() const
{
    
    std::string s1, s2, s3, op, type;
    s1 = operands[0]->toStr();
    s2 = operands[1]->toStr();
    s3 = operands[2]->toStr();
    type = operands[0]->getType()->toStr();
    switch (opcode)
    {
    case ADD:
        op = "add";
        break;
    case SUB:
        op = "sub";
        break;
    case MULT:
        op = "mul";
        break;
    case DIV:
        op = "sdiv";
        break;
    case MOD:
        op = "srem";
        break;
    case AND:
        op = "and";
        break;
    case XOR:
        op = "xor";
        break;
    case OR:
        op = "or";
        break;
    default:
        break;
    }
    fprintf(yyout, "  %s = %s %s %s, %s\n", s1.c_str(), op.c_str(), type.c_str(), s2.c_str(), s3.c_str());
}

void BinaryInstruction::genMachineCode(AsmBuilder* builder)
{
    auto cur_block = builder->getBlock();
    auto mfunc = builder->getFunction();
    auto dst = genMachineOperand(operands[0]);
    auto src1 = genMachineOperand(operands[1]);
    auto src2 = genMachineOperand(operands[2]);
    MachineInstruction* cur_inst = nullptr;
    if(src1->isImm())
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src1);
        cur_block->InsertInst(cur_inst);
        src1 = new MachineOperand(*internal_reg);
    }
    switch (opcode)
    {
    case ADD:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
        break;
    case SUB:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::SUB, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
        break;
    case MULT:
        if(src2->isImm()){
             //先将src mov到寄存器中
            auto internal_reg = genMachineVReg();
            int src_value = src2->getVal();
            if(src_value < 0 && src_value > -512){
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MVN, internal_reg, src2);
            }
            else if(src_value >= 0 && src_value < 256) {
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, internal_reg, src2);
            }
            else {
                cur_inst = new LoadMInstruction(cur_block, internal_reg, src2);
            }
            cur_block->InsertInst(cur_inst);
            src2 = new MachineOperand(*internal_reg);
        }
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::MULT, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
        break;
    case DIV:
    {
        if(src2->isImm()){
             //先将src mov到寄存器中
            auto internal_reg = genMachineVReg();
            int src_value = src2->getVal();
            if(src_value < 0 && src_value > -512){
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MVN, internal_reg, src2);
            }
            else if(src_value >= 0 && src_value < 256) {
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, internal_reg, src2);
            }
            else {
                cur_inst = new LoadMInstruction(cur_block, internal_reg, src2);
            }
            cur_block->InsertInst(cur_inst);
            src2 = new MachineOperand(*internal_reg);
        }
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::DIV, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
        break;
        /*auto dst_temp = genMachineReg(0);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src1);
        cur_block->InsertInst(cur_inst);
        src1 = dst_temp;
        if(src2->isImm()){
             //先将src mov到寄存器中
            auto dst_temp = genMachineReg(1);
            int src_value = src2->getVal();
            if(src_value < 0 && src_value > -512){
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MVN, dst_temp, src2);
            }
            else if(src_value >= 0) {
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src2);
            }
            else {
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src2);
                std::cout << "StoreInstruction::genMachineCode 其他范围数据" << std::endl;
            }
            cur_block->InsertInst(cur_inst);
            src2 = dst_temp;
        } else {
            auto dst_temp = genMachineReg(1);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src2);
            cur_block->InsertInst(cur_inst);
            src2 = dst_temp;
        }
        MachineOperand* div_label = new MachineOperand(".func__aeabi_idiv(PLT)");
        cur_inst = new BranchMInstruction(cur_block, BranchMInstruction::BL,div_label);
        cur_block->InsertInst(cur_inst);
        auto res_temp = genMachineReg(0);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, res_temp);
        cur_block->InsertInst(cur_inst);
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::DUMMY, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
        mfunc->setAsNonLeaf();*/
        break;
    }
    case MOD: {
        if(src2->isImm()){
             //先将src mov到寄存器中
            auto internal_reg = genMachineVReg();
            int src_value = src2->getVal();
            if(src_value < 0 && src_value > -512){
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MVN, internal_reg, src2);
            }
            else if(src_value >= 0 && src_value < 256) {
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, internal_reg, src2);
            }
            else {
                cur_inst = new LoadMInstruction(cur_block, internal_reg, src2);
            }
            cur_block->InsertInst(cur_inst);
            src2 = new MachineOperand(*internal_reg);
        }
        auto internal_reg_div = genMachineVReg();
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::DIV, internal_reg_div, src1, src2);
        cur_block->InsertInst(cur_inst);
        auto internal_reg_mult = genMachineVReg();
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::MULT, internal_reg_mult, internal_reg_div, src2);
        cur_block->InsertInst(cur_inst);
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::SUB, dst, src1, internal_reg_mult);
        cur_block->InsertInst(cur_inst);
        break;
        /*auto dst_temp = genMachineReg(0);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src1);
        cur_block->InsertInst(cur_inst);
        src1 = dst_temp;
        if(src2->isImm()){
             //先将src mov到寄存器中
            auto dst_temp = genMachineReg(1);
            int src_value = src2->getVal();
            if(src_value < 0 && src_value > -512){
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MVN, dst_temp, src2);
            }
            else if(src_value >= 0) {
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src2);
            }
            else {
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src2);
                std::cout << "StoreInstruction::genMachineCode 其他范围数据" << std::endl;
            }
            cur_block->InsertInst(cur_inst);
            src2 = dst_temp;
        } else {
            auto dst_temp = genMachineReg(1);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src2);
            cur_block->InsertInst(cur_inst);
            src2 = dst_temp;
        }
        MachineOperand* div_label = new MachineOperand(".func__aeabi_idiv(PLT)");
        cur_inst = new BranchMInstruction(cur_block,BranchMInstruction::BL,div_label);
        cur_block->InsertInst(cur_inst);
        auto res_temp = genMachineReg(1);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, res_temp);
        cur_block->InsertInst(cur_inst);
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::DUMMY, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
        mfunc->setAsNonLeaf();*/
        break;
    }
    case AND:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::AND, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
        break;
    case XOR:
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst,
            new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::EQ);
        cur_block->InsertInst(cur_inst);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst,
            new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::NE);
        cur_block->InsertInst(cur_inst);
        // cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::XOR, dst, src1, src2);
        // cur_block->InsertInst(cur_inst);
        break;
    case OR:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::OR, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
        break;
    default:
        break;
    }
}

CmpInstruction::CmpInstruction(unsigned opcode, Operand *dst, Operand *src1, Operand *src2, BasicBlock *insert_bb): Instruction(CMP, insert_bb){
    this->opcode = opcode;
    operands.push_back(dst);
    if(src1->getType()->isPTR()) {
        TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        Operand *new_dst = new SimpleOperand(temp_se);
        insert_bb->insertBefore(new LoadInstruction(new_dst, src1), this);
        src1 = new_dst;
        
    }
    if(src2->getType()->isPTR()) {
        TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        Operand *new_dst = new SimpleOperand(temp_se);
        insert_bb->insertBefore(new LoadInstruction(new_dst, src2), this);
        
        src2 = new_dst;
    }
    operands.push_back(src1);
    operands.push_back(src2);
    dst->setDef(this);
    src1->addUse(this);
    src2->addUse(this);
}

CmpInstruction::~CmpInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
    operands[2]->removeUse(this);
}

void CmpInstruction::output() const
{
    std::string s1, s2, s3, op, type;
    s1 = operands[0]->toStr();
    s2 = operands[1]->toStr();
    s3 = operands[2]->toStr();
    type = operands[1]->getType()->toStr();
    switch (opcode)
    {
    case E:
        op = "eq";
        break;
    case NE:
        op = "ne";
        break;
    case L:
        op = "slt";
        break;
    case LE:
        op = "sle";
        break;
    case G:
        op = "sgt";
        break;
    case GE:
        op = "sge";
        break;
    default:
        op = "";
        break;
    }
    
    fprintf(yyout, "  %s = icmp %s %s %s, %s\n", s1.c_str(), op.c_str(), type.c_str(), s2.c_str(), s3.c_str());
}

void CmpInstruction::genMachineCode(AsmBuilder* builder){
    auto cur_block = builder->getBlock();
    auto dst = genMachineOperand(operands[0]);
    auto src1 = genMachineOperand(operands[1]);
    auto src2 = genMachineOperand(operands[2]);
    MachineInstruction* cur_inst = nullptr;
    if(src1->isImm())
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src1);
        cur_block->InsertInst(cur_inst);
        src1 = new MachineOperand(*internal_reg);
    }
    if(src2->isImm() && src2->getVal() > 256)
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src2);
        cur_block->InsertInst(cur_inst);
        src2 = new MachineOperand(*internal_reg);
    }
    if(operands[1]->getEntry()->getType()->isBool()){
        SymbolEntry* dst_se = operands[1]->getEntry();
        Cond_inst ci = builder->search(dst_se);
        switch (ci.getOpCode())
        {
        case MachineInstruction::EQ:
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::EQ);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::NE);
            break;
        case MachineInstruction::NE:    
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::NE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::EQ);
            break;
        case MachineInstruction::LE:         
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::LE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::GT);
            break;
        case MachineInstruction::GT:        
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::GT);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::LE);
            break;
        case MachineInstruction::GE:       
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::GE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::LT);
            break;
        case MachineInstruction::LT:           
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::LT);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src1, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::GE);
            break;
        case MachineInstruction::NONE:{
            //!产生的bool->int类转换
            break;
        }
        default:
            break;
        }
        cur_block->InsertAfter(cur_inst ,ci.getInstruction());

    }
    if(operands[2]->getEntry()->getType()->isBool()){
        SymbolEntry* dst_se = operands[2]->getEntry();
        Cond_inst ci = builder->search(dst_se);
        switch (ci.getOpCode())
        {
        case MachineInstruction::EQ:
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::EQ);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::NE);
            break;
        case MachineInstruction::NE:    
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::NE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::EQ);
            break;
        case MachineInstruction::LE:         
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::LE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::GT);
            break;
        case MachineInstruction::GT:        
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::GT);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::LE);
            break;
        case MachineInstruction::GE:       
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::GE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::LT);
            break;
        case MachineInstruction::LT:           
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::LT);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, src2, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::GE);
            break;
        case MachineInstruction::NONE:{
            break;
        }
        default:
            break;
        }
        cur_block->InsertAfter(cur_inst ,ci.getInstruction());

    }
    int Cmp_opcode;
    switch (opcode)
    {
    case E:
        cur_inst = new CmpMInstruction(cur_block, src1, src2);
        Cmp_opcode = MachineInstruction::EQ;
        break;
    case NE:
        cur_inst = new CmpMInstruction(cur_block, src1, src2);
        Cmp_opcode = MachineInstruction::NE;
        break;
    case L:
        cur_inst = new CmpMInstruction(cur_block, src1, src2);
        Cmp_opcode = MachineInstruction::LT;
        break;
    case GE:
        cur_inst = new CmpMInstruction(cur_block, src1, src2);
        Cmp_opcode = MachineInstruction::GE;
        break;
    case G:
        cur_inst = new CmpMInstruction(cur_block, src1, src2);
        Cmp_opcode = MachineInstruction::GT;
        break;
    case LE:
        cur_inst = new CmpMInstruction(cur_block, src1, src2);
        Cmp_opcode = MachineInstruction::LE;
        
        break;
    default:
        break;
    }
    builder->setCmpOpcode(Cmp_opcode);
    cur_block->InsertInst(cur_inst);
    builder->insert_map(this->operands[0]->getEntry(), Cmp_opcode, cur_inst);
}
UncondBrInstruction::UncondBrInstruction(BasicBlock *to, BasicBlock *insert_bb) : Instruction(UNCOND, insert_bb)
{
    branch = to;
}

void UncondBrInstruction::output() const
{
    fprintf(yyout, "  br label %%B%d\n", branch->getNo());
}

void UncondBrInstruction::setBranch(BasicBlock *bb)
{
    branch = bb;
}

BasicBlock *UncondBrInstruction::getBranch()
{
    return branch;
}

void UncondBrInstruction::genMachineCode(AsmBuilder* builder){
    auto cur_block = builder->getBlock();
    MachineInstruction* cur_inst = new BranchMInstruction(cur_block, BranchMInstruction::B, genMachineLabel(branch->getNo()));
    cur_block->InsertInst(cur_inst);
}

CondBrInstruction::CondBrInstruction(BasicBlock*true_branch, BasicBlock*false_branch, Operand *cond, BasicBlock *insert_bb) : Instruction(COND, insert_bb){
    this->true_branch = true_branch;
    this->false_branch = false_branch;
    cond->addUse(this);
    operands.push_back(cond);
}

CondBrInstruction::~CondBrInstruction()
{
    operands[0]->removeUse(this);
}

void CondBrInstruction::output() const
{
    std::string cond, type;
    cond = operands[0]->toStr();
    type = operands[0]->getType()->toStr();
    int true_label = true_branch->getNo();
    int false_label = false_branch->getNo();
    fprintf(yyout, "  br %s %s, label %%B%d, label %%B%d\n", type.c_str(), cond.c_str(), true_label, false_label);
}

void CondBrInstruction::setFalseBranch(BasicBlock *bb)
{
    false_branch = bb;
}

BasicBlock *CondBrInstruction::getFalseBranch()
{
    return false_branch;
}

void CondBrInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    MachineInstruction* true_inst = new BranchMInstruction(cur_block, BranchMInstruction::B,
        genMachineLabel(true_branch->getNo()), builder->getCmpOpcode());
    cur_block->InsertInst(true_inst);
    // true branch, cond branch

    MachineInstruction* false_inst = new BranchMInstruction(cur_block, BranchMInstruction::B,
        genMachineLabel(false_branch->getNo()));
    cur_block->InsertInst(false_inst);
    // false branch, uncond branch

}

void CondBrInstruction::setTrueBranch(BasicBlock *bb)
{
    true_branch = bb;
}

BasicBlock *CondBrInstruction::getTrueBranch()
{
    return true_branch;
}

RetInstruction::RetInstruction(Operand *src, BasicBlock *insert_bb) : Instruction(RET, insert_bb)
{
    if(src != nullptr)
    {
        if(src->getType()->isPTR()) {
            TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            Operand *new_dst = new SimpleOperand(temp_se);
            insert_bb->insertBefore(new LoadInstruction(new_dst, src), this);
            src = new_dst;
        }
        operands.push_back(src);
        src->addUse(this);
    }
}

RetInstruction::~RetInstruction()
{
    if(!operands.empty())
        operands[0]->removeUse(this); 
}

void RetInstruction::output() const
{
    if(operands.empty())
    {
        fprintf(yyout, "  ret void\n");
    }
    else
    {
        std::string ret, type;
        ret = operands[0]->toStr();
        type = operands[0]->getType()->toStr();
        fprintf(yyout, "  ret %s %s\n", type.c_str(), ret.c_str());
    }
}

void RetInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    auto src = genMachineOperand(operands[0]);
    auto mfunc = builder->getFunction();
    MachineInstruction *inst;

    // Save return value
    inst = new MovMInstruction(cur_block, MovMInstruction::MOV,
        new MachineOperand(MachineOperand::REG, 0), src);
    cur_block->InsertInst(inst);

    inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD,
        new MachineOperand(MachineOperand::REG, 13),
        new MachineOperand(MachineOperand::REG, 11),
        new MachineOperand(MachineOperand::IMM, 0));
    cur_block->InsertInst(inst);

    inst = new BranchMInstruction(cur_block, BranchMInstruction::BX,
        new MachineOperand(MachineOperand::REG, 14));
    cur_block->InsertInst(inst);

    mfunc->addRetInst(inst);
}

AllocaInstruction::AllocaInstruction(Operand *dst, SymbolEntry *se, BasicBlock *insert_bb) : Instruction(ALLOCA, insert_bb)
{
    this->align = 4;
    operands.push_back(dst);
    dst->setDef(this);
    this->se = se;
    // if(se->getType()->isArray()) this->align = 16;
}
/*
AllocaInstruction::AllocaInstruction(Operand *dst, SymbolEntry *se , int align, BasicBlock *insert_bb) : Instruction(ALLOCA, insert_bb)
{
    this->align = align;
    operands.push_back(dst);
    dst->setDef(this);
    this->se = se;
}
*/
AllocaInstruction::~AllocaInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
}

void AllocaInstruction::output() const
{
    std::string dst, type;
    dst = operands[0]->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "  %s = alloca %s, align %d\n", dst.c_str(), type.c_str(), this->align);
}

void AllocaInstruction::genMachineCode(AsmBuilder* builder)
{
    if(se->getType()->isArray()){
    auto cur_func = builder->getFunction();
   
    ArrayType* arr_tp = (ArrayType*) se->getType();
    int space = arr_tp->getSize()*arr_tp->getItemCount();
    int offset = cur_func->AllocSpace(space); 
    SimpleOperand *ope = (SimpleOperand *)operands[0];
    dynamic_cast<TemporarySymbolEntry*>(ope->getEntry())->setOffset(-offset);
    return ;
    }
    /* HINT:
    * Allocate stack space for local variabel
    * Store frame offset in symbol entry */
    auto cur_func = builder->getFunction();
    int offset = cur_func->AllocSpace(4); 
    SimpleOperand *ope = (SimpleOperand *)operands[0];
    dynamic_cast<TemporarySymbolEntry*>(ope->getEntry())->setOffset(-offset);
}

LoadInstruction::LoadInstruction(Operand *dst, Operand *src_addr, BasicBlock *insert_bb) : Instruction(LOAD, insert_bb)
{
    operands.push_back(dst);
    operands.push_back(src_addr);
    dst->setDef(this);
    src_addr->addUse(this);
}

LoadInstruction::~LoadInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
}

void LoadInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    std::string src_type;
    std::string dst_type;
    dst_type = operands[0]->getType()->toStr();
    src_type = operands[1]->getType()->toStr();
    fprintf(yyout, "  %s = load %s, %s %s, align 4\n", dst.c_str(), dst_type.c_str(), src_type.c_str(), src.c_str());
}

void LoadInstruction::genMachineCode(AsmBuilder* builder)
{
    auto cur_block = builder->getBlock();
    MachineInstruction* cur_inst = nullptr;
    // Load global operand
    if(operands[1]->IsTrue()){
        
    }
    else if(operands[1]->IsZero()){

    }
    else if(operands[1]->IsSimple()){
        SimpleOperand* ope1 = (SimpleOperand*) operands[1];
        if(ope1->getEntry()->isVariable()
        && dynamic_cast<IdentifierSymbolEntry*>(ope1->getEntry())->isGlobal())
        {
            auto dst = genMachineOperand(operands[0]);
            auto internal_reg1 = genMachineVReg();
            auto internal_reg2 = new MachineOperand(*internal_reg1);
            auto src = genMachineOperand(operands[1]);
            // example: load r0, addr_a
            cur_inst = new LoadMInstruction(cur_block, internal_reg1, src);
            cur_block->InsertInst(cur_inst);
            // example: load r1, [r0]  
            cur_inst = new LoadMInstruction(cur_block, dst, internal_reg2);
            cur_block->InsertInst(cur_inst);
        }
        // Load local operand
        else if(ope1->getEntry()->isTemporary()
        && ope1->getDef()
        && ope1->getDef()->isAlloc())
        {
            // example: load r1, [r0, #4]
            auto dst = genMachineOperand(operands[0]);
            auto src1 = genMachineReg(11);
            auto src2 = genMachineImm(dynamic_cast<TemporarySymbolEntry*>(ope1->getEntry())->getOffset());
            cur_inst = new LoadMInstruction(cur_block, dst, src1, src2);
            cur_block->InsertInst(cur_inst);
        }
        // Load operand from temporary variable
        else
        {
            // example: load r1, [r0]
            auto dst = genMachineOperand(operands[0]);
            auto src = genMachineOperand(operands[1]);
            cur_inst = new LoadMInstruction(cur_block, dst, src);
            cur_block->InsertInst(cur_inst);
        }
    }
}

StoreInstruction::StoreInstruction(Operand *dst_addr, Operand *src, BasicBlock *insert_bb) : Instruction(STORE, insert_bb)
{
    if(src->getType()->isPTR()) {
            TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            Operand *new_dst = new SimpleOperand(temp_se);
            insert_bb->insertBefore(new LoadInstruction(new_dst, src), this);
            src = new_dst;
    }
    operands.push_back(dst_addr);
    operands.push_back(src);
    dst_addr->addUse(this);
    src->addUse(this);
}

StoreInstruction::~StoreInstruction()
{
    operands[0]->removeUse(this);
    operands[1]->removeUse(this);
}

void StoreInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    std::string dst_type = operands[0]->getType()->toStr();
    std::string src_type = operands[1]->getType()->toStr();
    fprintf(yyout, "  store %s %s, %s %s, align 4\n", src_type.c_str(), src.c_str(), dst_type.c_str(), dst.c_str());
}

void StoreInstruction::genMachineCode(AsmBuilder* builder)
{
    
    auto cur_block = builder->getBlock();
    MachineInstruction* cur_inst = nullptr; 
    auto dst = genMachineOperand(operands[0]);
    auto src = genMachineOperand(operands[1]);
    if(src->isImm()){
        //先将src mov到寄存器中
        auto internal_reg = genMachineVReg();
        int src_value = src->getVal();
        if(src_value < 0 && src_value > -512){
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MVN, internal_reg, src);
        }
        else if(src_value >= 0 && src_value < 256) {
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, internal_reg, src);
        }
        else {
            cur_inst = new LoadMInstruction(cur_block, internal_reg, src);
        }
        cur_block->InsertInst(cur_inst);
        src = new MachineOperand(*internal_reg);
    }
    if(operands[0]->getEntry()->isVariable() && dynamic_cast<IdentifierSymbolEntry*>(operands[0]->getEntry())->isGlobal()){
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, dst);
        cur_block->InsertInst(cur_inst);
        dst = internal_reg;
        cur_inst = new StoreMInstruction(cur_block, src, dst);
        cur_block->InsertInst(cur_inst);
    } else if(operands[0]->IsAddr) {
        cur_inst = new StoreMInstruction(cur_block, src, dst);
        cur_block->InsertInst(cur_inst);
    }
    else {
        auto dst_base = genMachineReg(11);
        auto dst_offset = genMachineImm(dynamic_cast<TemporarySymbolEntry*>(operands[0]->getEntry())->getOffset());
        cur_inst = new StoreMInstruction(cur_block, src, dst_base, dst_offset);
        cur_block->InsertInst(cur_inst);
    }
}

GlobalInitInstruction::GlobalInitInstruction(Operand *dst_addr, Operand *src, BasicBlock *insert_bb) : Instruction(GLOBAL_INIT, insert_bb)
{
    this->align = 4;
    operands.push_back(dst_addr);
    dst_addr->addUse(this);
    
    if(dst_addr->getType()->isPTR()) {
        PointerType* ptr_t = (PointerType* )dst_addr->getType();
        if(ptr_t->getptType()->isArray()) {   
            // align = 16;
            src = new Zeroinitializer(ptr_t->getptType());
        }
    }
    if(src->getType()->isPTR()) {
            TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            Operand *new_dst = new SimpleOperand(temp_se);
            insert_bb->insertBefore(new LoadInstruction(new_dst, src), this);
            src = new_dst;
        }
    src->addUse(this);
    operands.push_back(src);
}

GlobalInitInstruction::~GlobalInitInstruction()
{
    operands[0]->removeUse(this);
    operands[1]->removeUse(this);
}

void GlobalInitInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    std::string src_type = operands[1]->getType()->toStr();
    fprintf(yyout, " %s = dso_local global %s %s, align %d\n", dst.c_str(), src_type.c_str(), src.c_str(), align);
}

void GlobalInitInstruction::genMachineCode(AsmBuilder* builder) {
    if(operands[1]->IsInit()){
        //数组
        auto cur_block = builder->getBlock();
        MachineInstruction* cur_inst = nullptr;
        auto dst = genMachineOperand(operands[0]);
        PointerType* pt_tp = (PointerType*)operands[0]->getType();
        ArrayType* tp = (ArrayType*)(pt_tp->getptType());
        cur_inst = new ArrayGlobalMInstrcution(cur_block, dst, tp);
        builder->getUnit()->InsertGlobal(cur_inst);
    }
    else{
    auto cur_block = builder->getBlock();
    MachineInstruction* cur_inst = nullptr;
    auto dst = genMachineOperand(operands[0]);
    auto src = genMachineOperand(operands[1]);
    cur_inst = new GlobalMInstrcution(cur_block, dst, src);
    builder->getUnit()->InsertGlobal(cur_inst);
    }
}

void CallInstruction::output() const
{

    std::string dst = operands[0]->toStr();
    std::string cout_line = "";
    fprintf(yyout, "  %s", dst.c_str());
    cout_line += " = call ";
    FunctionType* func_type =(FunctionType*)func_se->getType();
    Type* rtn_type = func_type->getRetType();
    cout_line += rtn_type->toStr();
    cout_line += " ";
    cout_line += func_se->toStr();
    cout_line += "(";
    params.size();
    
    if(params.size() == 0){}
    else if(params.size() == 1){
        ExprNode* temp_expr = params[0];
        SimpleOperand* temp_op = (SimpleOperand*)ops[0];
        std::string p_type = temp_expr->getSymPtr()->getType()->toStr();
        // std::string p_name = temp_op->toStr();
        cout_line+=p_type;
        cout_line+=" ";
        fprintf(yyout, cout_line.c_str());
        fprintf(yyout, "%s", temp_op->toStr().c_str());
        cout_line = "";
        // cout_line+=p_name;
    }
    else{
        ExprNode* temp_expr = params[0];
        SimpleOperand* temp_op = (SimpleOperand*)ops[0];
        // std::string p_name = temp_op->toStr();
        std::string p_type = temp_expr->getSymPtr()->getType()->toStr();
        cout_line+=p_type;
        cout_line+=" ";
        fprintf(yyout, cout_line.c_str());
        fprintf(yyout, "%s", temp_op->toStr().c_str());
        cout_line = "";
        // cout_line+=p_name;
        for(int i = 1; i < params.size(); i++){
            cout_line+=",";
            temp_expr = params[i];
            temp_op = (SimpleOperand*)ops[i];
            p_type = temp_expr->getSymPtr()->getType()->toStr();
            // p_name = temp_op->toStr();
            cout_line+=p_type;
            cout_line+=" ";
            fprintf(yyout, cout_line.c_str());
            fprintf(yyout, "%s", temp_op->toStr().c_str());
            cout_line = "";
            // cout_line+=p_name;
        }
    }
    cout_line += ")\n";
    fprintf(yyout, cout_line.c_str());

}

void CallInstruction::genMachineCode(AsmBuilder *builder)
{
    std::vector<Operand*>::iterator it;
    MachineInstruction *inst;
    MachineOperand *src;
    MachineBlock *cur_block = builder->getBlock();
    MachineFunction *mfunc = builder->getFunction();
    mfunc->setAsNonLeaf();
    if (ops.size() > 4) {
        // param number more than 4
        int rnum = ops.size();
        std::vector<Operand*>::reverse_iterator rit = ops.rbegin();
        for (; rit != ops.rend(); rit++) {
            if (rnum > 4) {
                src = genMachineOperand(*rit);
                if (src->isImm()) {
                    auto dst_temp = new MachineOperand(MachineOperand::VREG, SymbolTable::getLabel());
                    int value = src->getVal();
                    
                    if (value < 0 && value > -512){
                        inst = new MovMInstruction(cur_block, MovMInstruction::MVN, dst_temp, src);
                    }
                    else if (value >= 0 && value < 256) {
                        inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst_temp, src);
                    }
                    else {
                        inst = new LoadMInstruction(cur_block, dst_temp, src);
                    }
                    cur_block->InsertInst(inst);
                    src = dst_temp;
                }
                inst = new StackMInstrcution(cur_block, StackMInstrcution::PUSH, src);
                cur_block->InsertInst(inst);
            }
            else {
                src = genMachineOperand(*rit);
                inst = new MovMInstruction(cur_block, MovMInstruction::MOV,
                    new MachineOperand(MachineOperand::REG, rnum - 1), src);
                cur_block->InsertInst(inst);
            }
            rnum--;
        }
        src = new MachineOperand(".func"+func_se->toStr());
        inst = new BranchMInstruction(cur_block, BranchMInstruction::BL, src);
        cur_block->InsertInst(inst);

        // recover sp
        inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD,
        new MachineOperand(MachineOperand::REG, 13),
        new MachineOperand(MachineOperand::REG, 13),
        new MachineOperand(MachineOperand::IMM, 4 * (ops.size() - 4)));
        cur_block->InsertInst(inst);

        inst = new MovMInstruction(cur_block, MovMInstruction::MOV,
            genMachineOperand(operands[0]), new MachineOperand(MachineOperand::REG, 0));
        cur_block->InsertInst(inst);
    }
    else {
        // param number less or equal to 4
        int rnum = 0;
        for (it = ops.begin(); it != ops.end(); it++){
            src = genMachineOperand(*it);
            inst = new MovMInstruction(cur_block, MovMInstruction::MOV,
                new MachineOperand(MachineOperand::REG, rnum++), src);
            cur_block->InsertInst(inst);
        }
        src = new MachineOperand(".func"+func_se->toStr());
        inst = new BranchMInstruction(cur_block, BranchMInstruction::BL, src);
        cur_block->InsertInst(inst);

        inst = new MovMInstruction(cur_block, MovMInstruction::MOV,
            genMachineOperand(operands[0]), new MachineOperand(MachineOperand::REG, 0));
        cur_block->InsertInst(inst);
    }
}

CallInstruction::CallInstruction(Operand *dst, SymbolEntry *func_se, std::vector<ExprNode*> params, std::vector<Operand*> ops, BasicBlock *insert_bb) : Instruction(CALL, insert_bb)
{
    this->ops = ops;
    operands.push_back(dst);
    dst->addUse(this);
    this->func_se = func_se;
    this->params = params;
}

CallInstruction::~CallInstruction()
{
    operands[0]->removeUse(this);
}

void GetIntInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    fprintf(yyout, "  %s = call i32 @getint()\n", dst.c_str());
}

void GetIntInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    MachineFunction *mfunc = builder->getFunction();
    mfunc->setAsNonLeaf();
    MachineInstruction* cur_inst = nullptr; 
    MachineOperand* func_label = new MachineOperand(".funcgetint(PLT)");
    MachineOperand* dst = genMachineOperand(operands[0]);
    cur_inst = new BranchMInstruction(cur_block,BranchMInstruction::BL,func_label);
    cur_block->InsertInst(cur_inst);
    auto res_temp = genMachineReg(0);
    cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, res_temp);
    cur_block->InsertInst(cur_inst);
}

GetIntInstruction::GetIntInstruction(Operand *dst, BasicBlock *insert_bb) : Instruction(GETINT_CALL, insert_bb)
{
    
    operands.push_back(dst);
    dst->addUse(this);
}

GetIntInstruction::~GetIntInstruction()
{
    operands[0]->removeUse(this);
}

void GetChInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    fprintf(yyout, "  %s = call i32 @getch()\n", dst.c_str());
}

void GetChInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    MachineFunction *mfunc = builder->getFunction();
    mfunc->setAsNonLeaf();
    MachineInstruction* cur_inst = nullptr; 
    MachineOperand* func_label = new MachineOperand(".funcgetch(PLT)");
    MachineOperand* dst = genMachineOperand(operands[0]);
    cur_inst = new BranchMInstruction(cur_block,BranchMInstruction::BL,func_label);
    cur_block->InsertInst(cur_inst);
    auto res_temp = genMachineReg(0);
    cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, res_temp);
    cur_block->InsertInst(cur_inst);
}

GetChInstruction::GetChInstruction(Operand *dst, BasicBlock *insert_bb) : Instruction(GETINT_CALL, insert_bb)
{
    
    operands.push_back(dst);
    dst->addUse(this);
}

GetChInstruction::~GetChInstruction()
{
    operands[0]->removeUse(this);
}
ZextInstruction::ZextInstruction(Operand *dst, Operand *src, BasicBlock *insert_bb) : Instruction(ZEXT, insert_bb)
{
    if(src->getType()->isPTR()) {
            TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            Operand *new_dst = new SimpleOperand(temp_se);
            insert_bb->insertBefore(new LoadInstruction(new_dst, src), this);
            src = new_dst;
        }
    operands.push_back(dst);
    operands.push_back(src);
    dst->setDef(this);
    src->addUse(this);
}

ZextInstruction::~ZextInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0]; 
    operands[1]->removeUse(this);
}

void ZextInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    std::string src_type;
    std::string dst_type;
    dst_type = operands[0]->getType()->toStr();
    src_type = operands[1]->getType()->toStr();
    fprintf(yyout, "  %s = zext %s %s to %s\n", dst.c_str(), src_type.c_str(), src.c_str(), dst_type.c_str());
}

void ZextInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    auto dst = genMachineOperand(operands[0]);
    auto src = genMachineOperand(operands[1]);
    MachineInstruction* cur_inst = nullptr; 
    //src是bool，dst是i32
    if(operands[0]->getEntry()->getType()->isInt() && operands[1]->getEntry()->getType()->isBool()){
        //获取dst的se
        SymbolEntry* dst_se = operands[1]->getEntry();
        Cond_inst ci = builder->search(dst_se);
        switch (ci.getOpCode())
        {
        case MachineInstruction::EQ:{
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::EQ);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::NE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            break;}
        case MachineInstruction::NE:  {  
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::NE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::EQ);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            break;}
        case MachineInstruction::LE:  {       
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::LE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::GT);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            break;}
        case MachineInstruction::GT:  {      
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::GT);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::LE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            break;}
        case MachineInstruction::GE:   {    
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::GE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::LT);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            break;}
        case MachineInstruction::LT:  {         
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 1), MachineInstruction::LT);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, new MachineOperand(MachineOperand::IMM, 0), MachineInstruction::GE);
            cur_block->InsertAfter(cur_inst ,ci.getInstruction());
            break;}
        case MachineInstruction::NONE:{
            //!产生的bool->int类转换
            auto cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src);
            cur_block->InsertInst(cur_inst);
            break;
        }
        default:
            break;
        }
    }
    else{
        auto cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, src);
        cur_block->InsertInst(cur_inst);
    }
    
}

void PutInstruction::output() const
{
    std::string src = operands[0]->toStr();
    if(kind == PUTINT)
        fprintf(yyout, "  call void @putint(i32 %s)\n", src.c_str());
    if(kind == PUTCH)
        fprintf(yyout, "  call void @putch(i32 %s)\n", src.c_str());
}

void PutInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    MachineFunction *mfunc = builder->getFunction();
    mfunc->setAsNonLeaf();
    MachineInstruction* cur_inst = nullptr; 
    MachineOperand* func_label;
    switch (kind)
    {
    case PUTINT:
        func_label = new MachineOperand(".funcputint(PLT)");
        break;
    case PUTCH:
        func_label = new MachineOperand(".funcputch(PLT)");
        break;
    default:
        break;
    }
    MachineOperand* src = genMachineOperand(operands[0]);
    auto param_reg = genMachineReg(0);
    if(src->isImm())
    {
        cur_inst = new LoadMInstruction(cur_block, param_reg, src);
        cur_block->InsertInst(cur_inst);
        src = new MachineOperand(*param_reg);
    }
    else{
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, param_reg, src);
        cur_block->InsertInst(cur_inst);
    }
    
    cur_inst = new BranchMInstruction(cur_block,BranchMInstruction::BL,func_label);
    cur_block->InsertInst(cur_inst);
}

PutInstruction::PutInstruction(Operand *src, BasicBlock *insert_bb, int kind) : Instruction(PUTINT_CALL, insert_bb)
{
    this->kind = kind;
    if(src->getType()->isPTR()) {
            TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            Operand *new_dst = new SimpleOperand(temp_se);
            insert_bb->insertBefore(new LoadInstruction(new_dst, src), this);
            src = new_dst;
        }
    operands.push_back(src);
    src->addUse(this);
}

PutInstruction::~PutInstruction()
{
    operands[0]->removeUse(this);
}

GetPtrInstruction::GetPtrInstruction(Operand *dst, Operand *src, Operand* offset, Type* ptr, BasicBlock *insert_bb) : Instruction(GETPTR, insert_bb)
{
    
    operands.push_back(dst);
    operands.push_back(src);
    operands.push_back(offset);
    dst->setDef(this);
    src->addUse(this);
    offset->addUse(this);
    this->ptr_type = ptr;
}

GetPtrInstruction::~GetPtrInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
    operands[2]->removeUse(this);
}

void GetPtrInstruction::output() const
{
    //待修缮
    //%3 = getelementptr inbounds [30 x [20 x i32]], [30 x [20 x i32]]* %2, i64 0, i64 11
    std::string dst = operands[0]->toStr();
    std::string src_type = ptr_type->toStr();
    // std::string dst_type = operands[0]->getType()->toStr();
    std::string dst_type = ((PointerType*)ptr_type)->getptType()->toStr();
    std::string src_pt_type = operands[1]->toStr();
    std::string offset = operands[2]->toStr();
    fprintf(yyout, "  %s = getelementptr inbounds %s, %s %s, i64 0, i64 %s\n", dst.c_str(), dst_type.c_str(), src_type.c_str(), src_pt_type.c_str(), offset.c_str());
}

void GetPtrInstruction::genMachineCode(AsmBuilder* builder)
{

    auto cur_block = builder->getBlock();
    auto mfunc = builder->getFunction();
    auto dst = genMachineOperand(operands[0]);
    auto src1 = genMachineOperand(operands[2]);
    
    //ConstantSymbolEntry* con_se = new ConstantSymbolEntry(TypeSystem::intType, ((ArrayType*)(this->ptr_type))->getSize());
    ConstantSymbolEntry* con_se = new ConstantSymbolEntry(TypeSystem::intType, ((ArrayType*)(((PointerType*)(this->ptr_type))->getptType()))->getSize());
    MachineInstruction* cur_inst = nullptr;
    auto src_base = genMachineOperand(operands[1]);
    if(src_base->isImm() || src_base->isLabel())
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src_base);
        cur_block->InsertInst(cur_inst);
        src_base = new MachineOperand(*internal_reg);
    }
    if((operands[1]->getEntry())->isTemporary())
    {   
        int offset = ((TemporarySymbolEntry*)operands[1]->getEntry())->getOffset();
        if(offset != 0){
        MachineOperand* offset_mop = new MachineOperand(MachineOperand::IMM, offset);
        auto internal_reg = genMachineVReg(); 
        cur_inst = new LoadMInstruction(cur_block, internal_reg, offset_mop);
        cur_block->InsertInst(cur_inst);
        src_base = new MachineOperand(*internal_reg);}
    }
    Operand* con_op = new SimpleOperand(con_se);
    auto src2 = genMachineOperand(con_op);
    
    if(src1->isImm())
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src1);
        cur_block->InsertInst(cur_inst);
        src1 = new MachineOperand(*internal_reg);
    }
    if(src2->isImm())
    {
        auto internal_reg = genMachineVReg();
        cur_inst = new LoadMInstruction(cur_block, internal_reg, src2);
        cur_block->InsertInst(cur_inst);
        src2 = new MachineOperand(*internal_reg);
    }

    auto temp_mult = genMachineVReg();
    cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::MULT, temp_mult, src1, src2);
    cur_block->InsertInst(cur_inst);
    auto src3 = genMachineOperand(operands[1]);
    cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, dst, temp_mult, src_base);
    cur_block->InsertInst(cur_inst);
    if((operands[1]->getEntry())->isTemporary())
    {   
        auto fp = genMachineReg(11);
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, dst, fp, dst);
        cur_block->InsertInst(cur_inst);
    }
}
