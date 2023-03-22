#include "Ast.h"
#include "SymbolTable.h"
#include "Unit.h"
#include "Instruction.h"
#include "IRBuilder.h"
#include <string>
#include "Type.h"
#include <list>
#include "Operand.h"
#include <stack>
extern FILE *yyout;
int Node::counter = 0;
IRBuilder* Node::builder = nullptr;
WhileStmt* now_while_stmt = nullptr;
Node::Node()
{
    seq = counter++;
}

void Node::backPatch(std::vector<Instruction*> &list, BasicBlock*bb, bool true_branch = true)
{
    for(auto &inst:list)
    {
        if(inst->isCond())
        {
            if (true_branch) 
                dynamic_cast<CondBrInstruction*>(inst)->setTrueBranch(bb);
            else 
                dynamic_cast<CondBrInstruction*>(inst)->setFalseBranch(bb);
        }
        else if(inst->isUncond())
            dynamic_cast<UncondBrInstruction*>(inst)->setBranch(bb);
    }
}

std::vector<Instruction*> Node::merge(std::vector<Instruction*> &list1, std::vector<Instruction*> &list2)
{
    std::vector<Instruction*> res(list1);
    res.insert(res.end(), list2.begin(), list2.end());
    return res;
}

void Ast::genCode(Unit *unit)
{
    IRBuilder *builder = new IRBuilder(unit);
    Node::setIRBuilder(builder);
    root->genCode();
    Instruction* init_head = builder->getUnit()->getGlobal_init();
    Instruction* init_last = init_head->getPrev();
    Instruction* main_block_head = builder->getUnit()->getMainFunc()->getEntry()->getHead();
    init_last->setNext(main_block_head->getNext());
    main_block_head->setNext(init_head);
    init_head->setPrev(main_block_head);
    main_block_head->setPrev(init_last);
}

void FunctionDef::genCode()
{
    
    Unit *unit = builder->getUnit();
    Function *func = new Function(unit, se);
    BasicBlock *entry = func->getEntry();
    // set the insert point to the entry basicblock of this function.
    builder->setInsertBB(entry);

    if(idList != nullptr) {
        ((IDList *)idList)->genCode();
    }

    if(this->se->toStr() == "main") {unit->setMainFunc(func);}
    stmt->genCode();
    /**
     * Construct control flow graph. You need do set successors and predecessors for each basic block.
     * Todo
    */
    
    BasicBlock *bb;
    BasicBlock *temp = func->getEntry();
    std::list<BasicBlock *> blocks;
    std::vector<BasicBlock *> used_blocks;

    blocks.push_back(temp);
    while(blocks.size() != 0)
    {
        bb = blocks.front();
        blocks.pop_front();
        Instruction *ins = bb->rbegin();
        if(std::find(used_blocks.begin(), used_blocks.end(), bb) == used_blocks.end()){
            if(ins->isCond())
            {
                //fprintf(yyout, "condSet!\n");
                BasicBlock *TrueBranch = ((CondBrInstruction *)ins)->getTrueBranch();
                BasicBlock *FalseBranch = ((CondBrInstruction *)ins)->getFalseBranch();
                bb->addSucc(TrueBranch);
                bb->addSucc(FalseBranch);
                TrueBranch->addPred(bb);
                FalseBranch->addPred(bb);
                blocks.push_back(TrueBranch);
                blocks.push_back(FalseBranch);
            }else if(ins->isUncond()){
                //fprintf(yyout, "uncondSet!\n");
                BasicBlock *UBranch = ((UncondBrInstruction *)ins)->getBranch();
                bb->addSucc(UBranch);
                UBranch->addPred(bb);
                blocks.push_back(UBranch);
            }
            else {
                //fprintf(yyout, "RetSet!\n");
            }
            used_blocks.push_back(bb);
        }
    }
}

void BinaryExpr::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Function *func = nullptr;
    if(bb != nullptr) func = bb->getParent();
    if(op == LOR)
    {
        BasicBlock *falsBB = new BasicBlock(func);
        expr1->genCode();
        Operand *addr = expr1->getOperand();
        if(addr->getType()->isInt()){
            SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
            Operand* conv = new SimpleOperand(se);
            new CmpInstruction(CmpInstruction::NE, conv, addr, zero_oprand, bb);
            addr = conv;
        }
        Instruction *inst1 = new CondBrInstruction(nullptr, falsBB, addr, builder->getInsertBB());
        backPatch(expr1->falseList(), falsBB, false);
        builder->setInsertBB(falsBB);
        expr2->genCode();
        dst = expr2->getOperand();
        // Instruction *inst2 = new CondBrInstruction(nullptr, nullptr, expr2->getOperand(), falsBB);
        false_list = expr2->falseList();
        true_list = merge(expr1->trueList(), expr2->trueList());
        // false_list.push_back(inst2);
        true_list.push_back(inst1);
        // true_list.push_back(inst2);
    }
    else if (op == LAND)
    {
        BasicBlock *trueBB = new BasicBlock(func);
        expr1->genCode();
        Operand *addr = expr1->getOperand();
        if(addr->getType()->isInt()){
            SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
            Operand* conv = new SimpleOperand(se);
            new CmpInstruction(CmpInstruction::NE, conv, addr, zero_oprand, bb);
            addr = conv;
        }
        Instruction *inst1 = new CondBrInstruction(trueBB, nullptr, addr, builder->getInsertBB());
        backPatch(expr1->trueList(), trueBB);
        builder->setInsertBB(trueBB);
        expr2->genCode();
        dst = expr2->getOperand();
        // Instruction *inst2 = new CondBrInstruction(nullptr, nullptr, expr2->getOperand(), trueBB);
        true_list = expr2->trueList();
        false_list = merge(expr1->falseList(), expr2->falseList());
        // true_list.push_back(inst2);
        false_list.push_back(inst1);
        // false_list.push_back(inst2);
    }
    else if(op >= LESS && op <= NOTEQUAL)
    {
        expr1->genCode();
        expr2->genCode();
        Operand *src1 = expr1->getOperand();
        Operand *src2 = expr2->getOperand();

        BasicBlock *bb = builder->getInsertBB();
        Operand* conv1, *conv2;
        if (src1->getType()->isBool()) {
            SymbolEntry *se1 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            conv1 = new SimpleOperand(se1);
            new ZextInstruction(conv1, src1, bb);
        } else {
            conv1 = src1;
        }
        if (src2->getType()->isBool()) {
            SymbolEntry *se2 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            conv2 = new SimpleOperand(se2);
            new ZextInstruction(conv2, src2, bb);
        } else {
            conv2 = src2;
        }

        int opcode;
        switch (op)
        {
        case LESS:
            opcode = CmpInstruction::L;
            break;
        case MORE:
            opcode = CmpInstruction::G;
            break;
        case LESSEQUAL:
            opcode = CmpInstruction::LE;
            break;
        case MOREEQUAL:
            opcode = CmpInstruction::GE;
            break;
        case EQUAL:
            opcode = CmpInstruction::E;
            break;
        case NOTEQUAL:
            opcode = CmpInstruction::NE;
            break;
        }
        
        new CmpInstruction(opcode, dst, conv1, conv2, bb);
        // Instruction *inst = new CondBrInstruction(nullptr, nullptr, dst, bb);
        // true_list.push_back(inst);
        // false_list.push_back(inst);
        
    }
    else if(op >= ADD && op <= MOD)
    {
        expr1->genCode();
        expr2->genCode();
        Operand *src1 = expr1->getOperand();
        Operand *src2 = expr2->getOperand();

        BasicBlock *bb = builder->getInsertBB();
        Operand* conv1, *conv2;
        if (src1->getType()->isBool()) {
            SymbolEntry *se1 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            conv1 = new SimpleOperand(se1);
            new ZextInstruction(conv1, src1, bb);
        } else {
            conv1 = src1;
        }
        if (src2->getType()->isBool()) {
            SymbolEntry *se2 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            conv2 = new SimpleOperand(se2);
            new ZextInstruction(conv2, src2, bb);
        } else {
            conv2 = src2;
        }
        
        int opcode;
        switch (op)
        {
        case ADD:
            opcode = BinaryInstruction::ADD;
            break;
        case SUB:
            opcode = BinaryInstruction::SUB;
            break;
        case MULT:
            opcode = BinaryInstruction::MULT;
            break;
        case DIV:
            opcode = BinaryInstruction::DIV;
            break;
        case MOD:
            opcode = BinaryInstruction::MOD;
            break;
        }
        new BinaryInstruction(opcode, dst, conv1, conv2, bb);
    }
    else if(op == AND){

    }
    else if(op == OR){

    }
}

void UnaryExpr::genCode()
{
    
    BasicBlock *bb = builder->getInsertBB();
    Function *func = nullptr;
    if(bb != nullptr) func = bb->getParent();
    if (op == NOT)
        {
        expr1->genCode();
        Operand *src1 = expr1->getOperand();
        if(expr1->getSymPtr()->getType()->isInt()){
            SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
            Operand* conv = new SimpleOperand(se);
            
            new CmpInstruction(CmpInstruction::NE, conv, src1, zero_oprand, bb);
            new BinaryInstruction(BinaryInstruction::XOR, dst, conv, true_oprand, bb);
            ((SimpleOperand*)dst)->setType(TypeSystem::boolType);
        }
        else if(expr1->getSymPtr()->getType()->isBool()){
            new BinaryInstruction(BinaryInstruction::XOR, dst, src1, true_oprand, bb);
            ((SimpleOperand*)dst)->setType(TypeSystem::boolType);
        }
        true_list = expr1->falseList();
        false_list = expr1->trueList();
    }
    else if(op == PAREN)
    {
        expr1->genCode();
        this->dst = expr1->getOperand();
        true_list = expr1->trueList();
        false_list = expr1->falseList();
    }
    else if(op >= MINUS && op <= PLUS)
    {
        switch (op)
        {
        case PLUS:// Todo nothing
        {
            expr1->genCode();
            Operand *src1 = expr1->getOperand();
            if(expr1->getSymPtr()->getType()->isBool()){
                SymbolEntry *conv_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
                Operand* conv = new SimpleOperand(conv_se);
                ((SimpleOperand*)dst)->setType(TypeSystem::intType);
                new ZextInstruction(dst, src1, bb);
            }
            else{
                this->dst = expr1->getOperand();
            }
            
            break;
        
        }case MINUS:
        {   expr1->genCode();
            Operand *src1 = expr1->getOperand();
            if(expr1->getSymPtr()->getType()->isBool()){
                SymbolEntry *conv_se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
                Operand* conv = new SimpleOperand(conv_se);
                new ZextInstruction(conv, src1, bb);
                new BinaryInstruction(BinaryInstruction::SUB, dst, zero_oprand, conv, bb);
            }
            else{
                 new BinaryInstruction(BinaryInstruction::SUB, dst, zero_oprand, src1, bb);
            }
            break;
            } 
        }
    }
}

void Constant::genCode()
{
    // we don't need to generate code.
}

void Id::genCode()
{
    if (symbolEntry->getType()->isArray()) {
        BasicBlock *bb = builder->getInsertBB();
        Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getAddr();
        ConstantSymbolEntry *cons_se = new ConstantSymbolEntry(TypeSystem::intType, 0);
        new GetPtrInstruction(dst, addr, new SimpleOperand(cons_se), new PointerType(addr->getType()), bb);
        
    }
    else {
        BasicBlock *bb = builder->getInsertBB();
        Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getAddr();
        new LoadInstruction(dst, addr, bb);
    }
}

void IfStmt::genCode()
{
    //fprintf(yyout, "If stmt!\n");
    Function *func;
    BasicBlock *then_bb, *end_bb;
    func = builder->getInsertBB()->getParent();
    then_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);

    // if (cond->getOperand() == nullptr) std::cout << "if null" << std::endl;
    cond->genCode();
    
    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), end_bb, false);
    // br

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(end_bb);
}

void IfElseStmt::genCode()
{
    // Todo
    Function *func;
    BasicBlock *then_bb, *else_bb, *end_bb;

    func = builder->getInsertBB()->getParent();
    then_bb = new BasicBlock(func);
    else_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);

    cond->genCode();
    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), else_bb, false);

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(else_bb);
    elseStmt->genCode();
    else_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, else_bb);
    
    builder->setInsertBB(end_bb);
}

void WhileStmt::genCode()
{
    Function *func;
    BasicBlock *cond_bb, *do_bb, *end_bb, *now_bb;

    now_bb = builder->getInsertBB();
    func = builder->getInsertBB()->getParent();
    cond_bb = new BasicBlock(func);
    do_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);
    this->end_bb = end_bb;
    this->cond_bb = cond_bb;
    new UncondBrInstruction(cond_bb, now_bb);

    builder->setInsertBB(cond_bb);
    cond->genCode();
    backPatch(cond->trueList(), do_bb);
    backPatch(cond->falseList(), end_bb, false);

    builder->setInsertBB(do_bb);
    doStmt->genCode();
    do_bb = builder->getInsertBB();
    new UncondBrInstruction(cond_bb, do_bb);

    builder->setInsertBB(end_bb);
}

void BreakStmt::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Function *func = bb->getParent();
    new UncondBrInstruction(this->within_while->getExt_bb(), bb);

}

void ContinueStmt::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Function *func = bb->getParent();
    new UncondBrInstruction(this->within_while->getCond_bb(), bb);
}

void EmptyStmt::genCode()
{
    // do nothing
}

void CompoundStmt::genCode()
{
    // Todo
    stmt->genCode();
}

void SeqNode::genCode()
{
    // Todo
    stmt1->genCode();
    stmt2->genCode();
}

void DeclStmt::genCode()
{
    //fprintf(yyout, "DeclStmt!\n");
    idList->genCode();
}

void IDList::genCode()
{
    
    InitID* ids = (InitID*)this->id;
    IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(ids->getSymPtr());
    
    if(se->isGlobal())
    {
        if(ids->getArraySize() != nullptr) {
            // Init Global Array
            BasicBlock *bb = builder->getInsertBB();
            Operand *addr;
            SymbolEntry *addr_se;
            addr_se = new IdentifierSymbolEntry(*se);
            addr_se->setType(new PointerType(se->getType()));
            addr = new SimpleOperand(addr_se);
            se->setAddr(addr);
            if(ids->have_Init()){
                InitArrayAssign* temp_initas = (InitArrayAssign*)ids;
                Instruction *globala = new GlobalInitInstruction(addr, zero_oprand);
                builder->getUnit()->insertFollow(globala);
                builder->getUnit()->global_collecting = true;
                // collecting
                Node *init_mtx = ((InitArrayAssign*)ids)->getMtx();
                Type *arrType = se->getType();
                Node *mtx_line = init_mtx;
                Node *mtx_list = nullptr;
                int n = ((ArrayType*)arrType)->getItemCount();
                for (int i = 0; i < n; i++) {
                    if (mtx_line != nullptr) {
                        mtx_list = ((ArrayInit*)mtx_line)->getList();
                    }
                    else {
                        mtx_list = nullptr;
                    }
                    TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(new PointerType(((ArrayType*)arrType)->getItemType()), SymbolTable::getLabel());
                    Operand *dst = new SimpleOperand(temp_se);
                    ConstantSymbolEntry *cons_se = new ConstantSymbolEntry(TypeSystem::intType, i);
                    new GetPtrInstruction(dst, addr, new SimpleOperand(cons_se), new PointerType(arrType), bb);
                    // Insert Inst
                    arrayInit(((ArrayType*)arrType)->getItemType(), dst, mtx_list);
                    dst->IsAddr = true;
                    if (mtx_line != nullptr) {
                        mtx_line = ((ArrayInit*)mtx_line)->getNext();
                    }
                }
                // end collecting
                builder->getUnit()->global_collecting = false;
            }
            else{
                Instruction *globala = new GlobalInitInstruction(addr, zero_oprand);
                builder->getUnit()->insertFollow(globala);
            }
        }
        else {
            Operand *addr;
            SymbolEntry *addr_se;
            addr_se = new IdentifierSymbolEntry(*se);
            addr_se->setType(new PointerType(se->getType()));
            addr = new SimpleOperand(addr_se);
            se->setAddr(addr);
            if(ids->have_Init()){
                InitAssign* temp_initas = (InitAssign*)ids;
                Instruction *globala = new GlobalInitInstruction(addr, zero_oprand);
                builder->getUnit()->insertFollow(globala);
                builder->getUnit()->global_collecting = true;
                temp_initas->getVal()->genCode();
                builder->getUnit()->global_collecting = false;
                Instruction *inita = new StoreInstruction(addr, temp_initas->getVal()->getOperand());
                builder->getUnit()->insert_init_Follow(inita);
            }
            else{
                Instruction *globala = new GlobalInitInstruction(addr, zero_oprand);
                builder->getUnit()->insertFollow(globala);
            }
        }
    }
    else if(se->isLocal())
    {
        if (ids->getArraySize() != nullptr) {
            Function *func = builder->getInsertBB()->getParent();
            BasicBlock *entry = func->getEntry();
            BasicBlock *bb = builder->getInsertBB();
            Instruction *alloca;
            Instruction *inita;
            Operand *addr;
            SymbolEntry *addr_se;
            Type *type;
            type = new PointerType(se->getType());
            addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
            addr = new SimpleOperand(addr_se);
            alloca = new AllocaInstruction(addr, se);                   // allocate space for local id in function stack.

            if(ids->have_Init()){
                Node *init_mtx = ((InitArrayAssign*)ids)->getMtx();
                Type *arrType = se->getType();
                Node *mtx_line = init_mtx;
                Node *mtx_list = nullptr;
                int n = ((ArrayType*)arrType)->getItemCount();
                for (int i = 0; i < n; i++) {
                    if (mtx_line != nullptr) {
                        mtx_list = ((ArrayInit*)mtx_line)->getList();
                    }
                    else {
                        mtx_list = nullptr;
                    }
                    TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(new PointerType(((ArrayType*)arrType)->getItemType()), SymbolTable::getLabel());
                    Operand *dst = new SimpleOperand(temp_se);
                    ConstantSymbolEntry *cons_se = new ConstantSymbolEntry(TypeSystem::intType, i);
                    new GetPtrInstruction(dst, addr, new SimpleOperand(cons_se), new PointerType(arrType), bb);
                    // Insert Inst
                    arrayInit(((ArrayType*)arrType)->getItemType(), dst, mtx_list);
                    if (mtx_line != nullptr) {
                        mtx_line = ((ArrayInit*)mtx_line)->getNext();
                    }
                }
            }
            entry->insertFront(alloca);
            se->setAddr(addr);
        }
        else {
            // fprintf(yyout, "local scope is: %d\n", se->getScope());
            Function *func = builder->getInsertBB()->getParent();
            BasicBlock *entry = func->getEntry();
            BasicBlock *bb = builder->getInsertBB();
            Instruction *alloca;
            Instruction *inita;
            Operand *addr;
            SymbolEntry *addr_se;
            Type *type;
            type = new PointerType(se->getType());
            addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
            addr = new SimpleOperand(addr_se);
            alloca = new AllocaInstruction(addr, se);                   // allocate space for local id in function stack.

            if(ids->have_Init()){
                InitAssign* temp_initas = (InitAssign*)ids;
                temp_initas->getVal()->genCode();
                inita = new StoreInstruction(addr, temp_initas->getVal()->getOperand(), bb);
            }
            entry->insertFront(alloca);
            se->setAddr(addr);      
        }                                    // set the addr operand in symbol entry so that we can use it in subsequent code generation.
    }
    else if(se->isParam()){
        // fprintf(yyout, "param scope is: %d\n", se->getScope());

        Operand *param_addr, *addr;
        SymbolEntry *param_addr_se, *addr_se;
        Type *param_type, *type;
        Function *func = builder->getInsertBB()->getParent();
        BasicBlock *entry = func->getEntry();
        Instruction *alloca;
        Instruction *inita;

        // param_type = new PointerType(se->getType());
        param_type = se->getType();
        param_addr_se = new TemporarySymbolEntry(param_type, SymbolTable::getLabel());
        param_addr = new SimpleOperand(param_addr_se);
        se->setParamAddr(param_addr);
        se->setLabel(((TemporarySymbolEntry *)param_addr_se)->getLabel());

        type = new PointerType(se->getType());
        addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
        addr = new SimpleOperand(addr_se);
        alloca = new AllocaInstruction(addr, se);
        inita = new StoreInstruction(addr, param_addr);

        entry->insertFront(inita);
        entry->insertFront(alloca);
        se->setAddr(addr);
    }

    if(this->prev != nullptr){
        this->prev->genCode();
    }
}
void IDList::arrayInit(Type *itType, Operand *src, Node* mtx_item)
{
    BasicBlock *bb = builder->getInsertBB();
    
    if (itType->isArray()) {
        int n = ((ArrayType*)itType)->getItemCount();
        Node *mtx_line = mtx_item;
        Node *mtx_list = nullptr;
        for (int i = 0; i < n; i++) {
            if (mtx_line != nullptr) {
                mtx_list = ((ArrayInit*)mtx_line)->getList();
            }
            else {
                mtx_list = nullptr;
            }
            TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(new PointerType(((ArrayType*)itType)->getItemType()), SymbolTable::getLabel());
            Operand *dst = new SimpleOperand(temp_se);
            ConstantSymbolEntry *cons_se = new ConstantSymbolEntry(TypeSystem::intType, i);
            new GetPtrInstruction(dst, src, new SimpleOperand(cons_se), new PointerType(itType), bb);
            arrayInit(((ArrayType*)itType)->getItemType(), dst, mtx_list);
            if (mtx_line != nullptr) {
                mtx_line = ((ArrayInit*)mtx_line)->getNext();
            }
        }
    }
    else {
        if (mtx_item != nullptr) {
            mtx_item->genCode();
            new StoreInstruction(src, ((ExprNode*)mtx_item)->getOperand(), bb);
        }
        else {
            ConstantSymbolEntry *cons_se = new ConstantSymbolEntry(TypeSystem::intType, 0);
            new StoreInstruction(src, new SimpleOperand(cons_se), bb);
        }
    }
    
}

void ReturnStmt::genCode()
{
    
    // Todo
    BasicBlock *bb = builder->getInsertBB();
    retValue->genCode();
    new RetInstruction(retValue->getOperand(), bb);
}

void Cond::genCode()
{
    // dst = new SimpleOperand(symbolEntry);
    lexpr->genCode();
    Operand *src1 = lexpr->getOperand();
    BasicBlock *bb = builder->getInsertBB();
    if(src1->getType()->isInt()){
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        Operand* conv = new SimpleOperand(se);
        
        new CmpInstruction(CmpInstruction::NE, conv, src1, zero_oprand, bb);
        // new BinaryInstruction(BinaryInstruction::EQUAL, dst, conv, true_oprand, bb);
        dst = conv;
    }
    else if(src1->getType()->isBool()){
        // if (dst == nullptr) std::cout << "jijiji" << std::endl;
        // new BinaryInstruction(BinaryInstruction::EQUAL, dst, src1, true_oprand, bb);
        dst = src1;
    }
    
    Instruction *inst = new CondBrInstruction(nullptr, nullptr, dst, bb);
    true_list = lexpr->trueList();
    false_list = lexpr->falseList();

    true_list.push_back(inst);
    false_list.push_back(inst);
}

void FuncCall::genCode()
{
    
    BasicBlock *bb = builder->getInsertBB();
    std::vector<ExprNode*> call_param_list;
    std::vector<Operand*> op_list;
    ParamList* temp_pointer = (ParamList*)params;
    while(temp_pointer != nullptr){
        ExprNode* temp_expr_node = (ExprNode*)temp_pointer->getValue();
        temp_expr_node->genCode();
        op_list.push_back(temp_expr_node->getOperand());
        call_param_list.push_back(temp_expr_node);
        temp_pointer = (ParamList*)temp_pointer->getPrev();
    }
    new CallInstruction(dst, FuncSe, call_param_list, op_list, bb);
}

void AssignStmt::genCode()
{
    if(lval->getSymPtr()->getType()->isArray()){
        //数组
        lval->genCode();
        BasicBlock *bb = builder->getInsertBB();
        expr->genCode();
        ArrayId* lval_ptr = (ArrayId*)lval;
        Operand *addr = lval_ptr->getOperand();
        Operand *src = expr->getOperand();
        new StoreInstruction(addr, src, bb);
        addr->IsAddr = true;
        return;
    }
    BasicBlock *bb = builder->getInsertBB();
    expr->genCode();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(lval->getSymPtr())->getAddr();
    Operand *src = expr->getOperand();
    new StoreInstruction(addr, src, bb);
}

void ForStmt::genCode()
{
}

void ExpStmt::genCode()
{
    expr->genCode();
}

void InitID::genCode()
{
}

void InitAssign::genCode()
{
}

void ParamList::genCode()
{
    val->genCode();
}

void SysFuncCall::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    if(kind == PUTINT){
        ParamList* temp_pointer = (ParamList*)params;
        ExprNode* temp_expr_node = (ExprNode*)temp_pointer->getValue();
        temp_expr_node->genCode();
        SimpleOperand* temp_op = (SimpleOperand*)temp_expr_node->getOperand();
        new PutInstruction(temp_op, bb, PutInstruction::PUTINT);
    }
    else if(kind == GETINT){
        new GetIntInstruction(dst, bb);
    }
    else if(kind == GETCH){
        new GetChInstruction(dst, bb);
    }
    else if(kind == PUTCH){
        ParamList* temp_pointer = (ParamList*)params;
        ExprNode* temp_expr_node = (ExprNode*)temp_pointer->getValue();
        temp_expr_node->genCode();
        SimpleOperand* temp_op = (SimpleOperand*)temp_expr_node->getOperand();
        new PutInstruction(temp_op, bb, PutInstruction::PUTCH);
    }
}

void ConstDeclStmt::genCode()
{
    idList->genCode();
}

void BinaryExpr::typeCheck()
{
    //变量未声明？应该不在这里解决
    Type* type1 = expr1->getSymPtr()->getType();
    Type* type2 = expr2->getSymPtr()->getType();
    //首先判断运算符
    //运算符是比较类型，本节点类型应为bool
    switch (op)
    {
    case LESS:
    case MORE:
    case LESSEQUAL:
    case MOREEQUAL:
    case EQUAL:
    case NOTEQUAL:{
        //只支持整形、浮点型的比较
        if((type1->isInt() || type1->isFloat()) && (type2->isInt() || type2->isFloat())){
            BoolType* this_type = new BoolType();
            this->getSymPtr()->setType(this_type);
        }
        else{
            fprintf(stderr, "type %s and %s mismatch, operator: compare",    
            type1->toStr().c_str(), type2->toStr().c_str());
        }
        break;

    }
    case ADD:
    case SUB:
    case MULT:
    case DIV:
    case MOD:{
        if(type1->isInt() && type2->isInt()){
            IntType* type_temp = (IntType*)type1;
            IntType* this_type = new IntType(type_temp->getSize());
            this->getSymPtr()->setType(this_type);
        }
        else if(type1->isFloat() && type2->isFloat()){
            FloatType* this_type = new FloatType(32);
            this->getSymPtr()->setType(this_type);
        }
        else{//暂时不允许bool参与算数运算
            fprintf(stderr, "type %s and %s mismatch, operator: int/float compute\n",    
            type1->toStr().c_str(), type2->toStr().c_str());
        }
        break;
    }
    case LOR:
    case LAND:{
        //不允许非bool表达式参与逻辑运算
        if(type1->isBool() && type2->isBool()){
            BoolType* this_type = new BoolType();
            this->getSymPtr()->setType(this_type);
        }
        else{
            fprintf(stderr, "type %s and %s mismatch, operator: logic operation\n",    
            type1->toStr().c_str(), type2->toStr().c_str());
        }
        break;
    }
    case AND:
    case OR: {
        //不允许不同类型表达式进行位运算
        if(type1==type2){
            this->getSymPtr()->setType(type1);
        }
        else{
            fprintf(stderr, "type %s and %s mismatch, operator: bit compute\n",    
            type1->toStr().c_str(), type2->toStr().c_str());
        }
        break;
    }
        break;
    default:
        break;

    }

    if(expr1 != nullptr)
        expr1->typeCheck();
    if(expr2 != nullptr)
        expr2->typeCheck();
}

void UnaryExpr::typeCheck()
{
    Type* type = expr1->getSymPtr()->getType();
    //首先判断运算符
    //运算符是比较类型，本节点类型应为bool
    switch (op){
        case NOT:{
            if(type->isBool()){
                BoolType* this_type = new BoolType();
                this->getSymPtr()->setType(this_type);
            }
            else{
                fprintf(stderr, "type %s mismatch, operator: logic operation\n",    
                type->toStr().c_str());
            }
        break;
        }
        case PAREN:{
            this->getSymPtr()->setType(type);
            break;
        }
        case MINUS:
        case PLUS:{
            if(type->isInt() || type->isFloat()){
                this->getSymPtr()->setType(type);
            }
            else{
                fprintf(stderr, "type %s mismatch, operator: compute\n",    
                type->toStr().c_str());
            }
            break;
        }
        default:
            break;
    }
    //变量未声明？应该不在这里解决

    //条件判断表达式 int 至 bool 类型隐式转换

    //数值运算表达式运算数类型是否正确 (可能导致此错误的情况有返回值为 void 的函数调用结果参与了某表达式计算)

    if(expr1 != nullptr)
        expr1->typeCheck();
}

void Constant::typeCheck()
{
    
}

void Id::typeCheck()
{
    
}

void EmptyStmt::typeCheck()
{
    
}

void CompoundStmt::typeCheck()
{
    if(stmt != nullptr)
        stmt->typeCheck();
}

void SeqNode::typeCheck()
{
    if(stmt1 != nullptr)
        stmt1->typeCheck();
    if(stmt2 != nullptr)
        stmt2->typeCheck();
}

void IfStmt::typeCheck()
{
    if(cond != nullptr)
        cond->typeCheck();
    if(thenStmt != nullptr)
        thenStmt->typeCheck();

    //检查cond类型
    Type* cond_type = cond->getSymPtr()->getType();
    if(!cond_type->isBool()){
        fprintf(stderr, "type %s mismatch, if statement condition error!\n",    
        cond_type->toStr().c_str());
    }
}

void IfElseStmt::typeCheck()
{
    if(cond != nullptr)
        cond->typeCheck();
    if(thenStmt != nullptr)
        thenStmt->typeCheck();
    if(elseStmt != nullptr)
        elseStmt->typeCheck();

    //检查cond类型  
    Type* cond_type = cond->getSymPtr()->getType();
    if(!cond_type->isBool()){
        fprintf(stderr, "type %s mismatch, if statement condition error!\n",    
        cond_type->toStr().c_str());
    }
}

void WhileStmt::typeCheck()
{
    WhileStmt* last_while = now_while_stmt;
    now_while_stmt = this;
    if(cond != nullptr)
        cond->typeCheck();
    if(doStmt != nullptr)
        doStmt->typeCheck();

    //检查cond类型  
    Type* cond_type = cond->getSymPtr()->getType();
    if(!cond_type->isBool()){
        fprintf(stderr, "type %s mismatch, while statement condition error!\n",    
        cond_type->toStr().c_str());
    }
    now_while_stmt = last_while;
}

void ForStmt::typeCheck()
{
    if(iniStmt != nullptr)
        iniStmt->typeCheck();
    if(cond != nullptr)
        cond->typeCheck();
    if(incStmt != nullptr)
        incStmt->typeCheck();
    if(doStmt != nullptr)
        doStmt->typeCheck();

    //检查cond类型  
    Type* cond_type = cond->getSymPtr()->getType();
    if(!cond_type->isBool()){
        fprintf(stderr, "type %s mismatch, while statement condition error!\n",    
        cond_type->toStr().c_str());
    }    
}



void ReturnStmt::typeCheck()
{
    if(retValue != nullptr)
        retValue->typeCheck();
        //突然发现有些时候应该先递归 后写相关的函数 自己看着办ovo

    //类型检查是否与声明相符
}

void BreakStmt::typeCheck()
{
    if(now_while_stmt == nullptr) {std::cout<<"not in while!!" << std::endl;return;}
    this->within_while = now_while_stmt;
}

void ContinueStmt::typeCheck()
{
    if(now_while_stmt == nullptr) {std::cout<<"not in while!!" << std::endl;return;}
    this->within_while = now_while_stmt;
}

void ExpStmt::typeCheck()
{
    if(expr != nullptr)
        expr->typeCheck();
}

void AssignStmt::typeCheck()
{
    if(lval != nullptr)
        lval->typeCheck();
    if(expr != nullptr)
        expr->typeCheck();
    //不是变量
    if(!lval->getSymPtr()->isVariable()){
        fprintf(stderr, "assignstmt left value isn't variable!\n");
    }
    else{
        IdentifierSymbolEntry* idse = (IdentifierSymbolEntry*)lval->getSymPtr();
        //是常量
        if(idse->isConst()){
            fprintf(stderr, "assignstmt left value is const!\n");
        }
    }
}

void InitID::typeCheck()
{
    if(array_size != nullptr){
        array_size->typeCheck();
        Node* ptr = array_size; 
        Type* se_type = TypeSystem::intType;
        std::stack<int> q;
        while (ptr)
        {
            ArraySize* a_p = (ArraySize*)ptr;
            q.push(a_p->getCpdSize());
            ptr = a_p->getPrev();
            
        }
        while(!q.empty()) {
            int s = q.top();
            q.pop();
            Type* new_type = new ArrayType(se_type, s);
            se_type = new_type;
        }
        if (this->getSymPtr()->getType()->isPTR()) {
            se_type = new PointerType(se_type);
        }
        //赋值
        this->getSymPtr()->setType(se_type);
        // std::cout << se_type->toStr() << std::endl;
    }
}

void InitAssign::typeCheck()
{
    if(val != nullptr)
        val->typeCheck();
    if(this->getSymPtr()->isVariable()){
        IdentifierSymbolEntry* ide_se = (IdentifierSymbolEntry*)this->getSymPtr();
        if(ide_se->isConst()){
            int const_v = this->val->ComputeValue();
            ide_se->setConstValue(const_v);
        }
    }
    if(array_size != nullptr)
        array_size->typeCheck();
}

void IDList::typeCheck()
{
    //id有两种可能：InitID和InitAssign此处递归会调用谁的重载？ 都可能被调用（类比output）请放心递归
    if(id != nullptr)
        id->typeCheck();
    if(prev != nullptr)
        prev->typeCheck();
}

void ParamList::typeCheck()
{
    //ParamList会调用谁的重载我就不知道了（不是我写的，不熟捏）
    if(val != nullptr)
        val->typeCheck();
    if(prev != nullptr)
        prev->typeCheck();
}

void Cond::typeCheck()
{
    // if(dst == nullptr) {
    //     fprintf(stderr, "cond dst null!\n");
    // }
}

void FuncCall::typeCheck()
{
    //不符合重载要求的函数重复声明

    if(params != nullptr)
        params->typeCheck();
}

void SysFuncCall::typeCheck()
{
    //不符合重载要求的函数重复声明
    
    if(params != nullptr)
        params->typeCheck();
}

void DeclStmt::typeCheck()
{
    //1.变量在同一作用域下重复声明
    //2.初始化类型错误


    if(idList != nullptr)
        idList->typeCheck();
}

void ConstDeclStmt::typeCheck()
{
    //1.变量在同一作用域下重复声明
    //2.初始化类型错误

    if(idList != nullptr)
        idList->typeCheck();
}

void FunctionDef::typeCheck()
{
    //不符合重载要求的函数重复声明
    if (idList != nullptr) {
        idList->typeCheck();
    }
    //递归
    if(stmt != nullptr)
        stmt->typeCheck();
}

void Ast::typeCheck()
{
    if(root != nullptr)
        root->typeCheck();
}

void Ast::output()
{
    fprintf(yyout, "program\n");
    if(root != nullptr)
        root->output(4);
}

void BinaryExpr::output(int level)
{
    std::string op_str;
    switch(op)
    {
        case ADD:
            op_str = "add";
            break;
        case SUB:
            op_str = "sub";
            break;
        case MULT:
            op_str = "mult";
            break;
        case DIV:
            op_str = "div";
            break;
        case MOD:
            op_str = "mod";
            break;
        case AND:
            op_str = "and";
            break;
        case OR:
            op_str = "or";
            break;
        case MORE:
            op_str = "more";
            break;
        case LESS:
            op_str = "less";
            break;
        case LESSEQUAL:
            op_str = "lessequal";
            break;
        case MOREEQUAL:
            op_str = "moreequal";
            break;
        case EQUAL:
            op_str = "equal";
            break;    
        case NOTEQUAL:
            op_str = "notequal";
            break;  
        case LOR:
            op_str = "lor";
            break;  
        case LAND:
            op_str = "land";
            break;  
    }
    fprintf(yyout, "%*cBinaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr1->output(level + 4);
    expr2->output(level + 4);
}

void UnaryExpr::output(int level)
{
    std::string op_str;
    switch(op)
    {
        case NOT:
            op_str = "not";
            break;
        case PAREN:
            op_str = "paren";
            break;
        case MINUS:
            op_str = "minus";
            break;
        case PLUS:
            op_str = "plus";
            break;
    }
    fprintf(yyout, "%*cUnaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr1->output(level + 4);
}

void Constant::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr();
    if(type == "int")
    {
    fprintf(yyout, "%*cIntegerLiteral\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
    }
    else if(type == "float")
    {
        fprintf(yyout, "%*cFloatLiteral\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
    }
    
}

void Id::output(int level)
{
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getScope();
    fprintf(yyout, "%*cId\tname: %s\tscope: %d\ttype: %s\n", level, ' ',
            name.c_str(), scope, type.c_str());
}

void Cond::output(int level)
{
    fprintf(yyout, "Cond Node\n");
}

void CompoundStmt::output(int level)
{
    fprintf(yyout, "%*cCompoundStmt\n", level, ' ');
    stmt->output(level + 4);
}

void SeqNode::output(int level)
{
    fprintf(yyout, "%*cSequence\n", level, ' ');
    stmt1->output(level + 4);
    stmt2->output(level + 4);
}

void IfStmt::output(int level)
{
    fprintf(yyout, "%*cIfStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
}

void WhileStmt::output(int level)
{
    fprintf(yyout, "%*cWhileStmt\n", level, ' ');
    cond->output(level + 4);
    doStmt->output(level + 4);
}

void IfElseStmt::output(int level)
{
    fprintf(yyout, "%*cIfElseStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
    elseStmt->output(level + 4);
}

void ReturnStmt::output(int level)
{
    fprintf(yyout, "%*cReturnStmt\n", level, ' ');
    retValue->output(level + 4);
}

void ContinueStmt::output(int level)
{
    fprintf(yyout, "%*cContinueStmt\n", level, ' ');
}

void BreakStmt::output(int level)
{
    fprintf(yyout, "%*cBreakStmtn", level, ' ');
}

void AssignStmt::output(int level)
{
    fprintf(yyout, "%*cAssignStmt\n", level, ' ');
    lval->output(level + 4);
    expr->output(level + 4);
}

void ExpStmt::output(int level)
{
    fprintf(yyout, "%*cExpStmt\n", level, ' ');
    expr->output(level + 4);
}

void InitAssign::output(int level)
{
    fprintf(yyout, "%*cInitAssign\n", level, ' ');   
    fprintf(yyout, "%*cId\tname: %s\n", level + 4, ' ', this->name);
    val->output(level + 4);
}

void InitID::output(int level)
{
    fprintf(yyout, "%*cId\tname: %s\n", level, ' ', this->name);
}

void IDList::output(int level)
{
    id->output(level);
    if(this->prev != nullptr){
        this->prev->output(level);
    }
}

void DeclStmt::output(int level)
{
    fprintf(yyout, "%*cDeclStmt\n", level, ' ');
    idList->output(level + 4);
}

void ConstDeclStmt::output(int level)
{
    fprintf(yyout, "%*cConstDeclStmt\n", level, ' ');
    idList->output(level + 4);
}

void FunctionDef::output(int level)
{
    std::string name, type;
    name = se->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "%*cFunctionDefine function name: %s, type: %s\n", level, ' ', 
            name.c_str(), type.c_str());
    if(idList != nullptr)
        idList->output(level + 4);
    stmt->output(level + 4);
}

void ParamList::output(int level)
{
    val->output(level);
    if(this->prev != nullptr){
        this->prev->output(level);
    }
}

void FuncCall::output(int level)
{
    std::string name, type;
    name = FuncSe->toStr();
    type = symbolEntry->getType()->toStr();
    fprintf(yyout, "%*cFunctionCall function name: %s, type: %s\n", level, ' ', 
            name.c_str(), type.c_str());
}

void SysFuncCall::output(int level)
{
    std::string name, type;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    fprintf(yyout, "%*cSysyLibCall function name: %s, type: %s\n", level, ' ', 
            name.c_str(), type.c_str());
}

void EmptyStmt::output(int level)
{
    fprintf(yyout, "%*cEmptyStmt\n", level, ' ');
}

void ArraySize::output(int level)
{
    int cnt = 1;
    Node *tempn = prev;
    while (tempn != nullptr) {
        cnt++;
        tempn = ((ArraySize *)tempn)->getPrev();
    }
    fprintf(yyout, "%*cArrayDimension: %i\n", level, ' ', cnt);
}

void ArraySize::typeCheck()
{
    if(prev != nullptr) 
        prev->typeCheck();
    ComputedSize = size->ComputeValue();
}

void ArraySize::genCode()
{

}

void ArrayInit::output(int level)
{
    fprintf(yyout, "%*cArray Initial Matrix\n", level, ' ');
}

void ArrayInit::typeCheck()
{

}

void ArrayInit::genCode()
{

}

void InitArrayAssign::output(int level)
{
    fprintf(yyout, "%*cInitArrayAssign\n", level, ' ');   
    fprintf(yyout, "%*cId\tname: %s\n", level + 4, ' ', this->name);
    array_size->output(level + 4);
    init_mtx->output(level + 4);
}

void InitArrayAssign::typeCheck()
{
    if(array_size != nullptr){
        array_size->typeCheck();
        Node* ptr = array_size; 
        Type* se_type = TypeSystem::intType;
        std::stack<int> q;
        while (ptr)
        {
            ArraySize* a_p = (ArraySize*)ptr;
            q.push(a_p->getCpdSize());
            ptr = a_p->getPrev();
            
        }
        while(!q.empty()) {
            int s = q.top();
            q.pop();
            Type* new_type = new ArrayType(se_type, s);
            se_type = new_type;
        }
        //赋值
        this->getSymPtr()->setType(se_type);
        // std::cout << se_type->toStr() << std::endl;
    }
}

void InitArrayAssign::genCode()
{

}

void ArrayId::output(int level)
{
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getScope();
    fprintf(yyout, "%*cArray\tname: %s\tscope: %d\ttype: %s\n", level, ' ',
            name.c_str(), scope, type.c_str());
}

void ArrayId::typeCheck()
{

}

void ArrayId::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    ArraySize *ofnode = (ArraySize*)offset;
    ArrayType *now_type = (ArrayType*)(symbolEntry->getType());
    TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(new PointerType(now_type->getItemType()), SymbolTable::getLabel());
    Operand *last_op = new SimpleOperand(temp_se);
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getAddr();
    if (((PointerType*)(addr->getType()))->getptType()->isPTR()) {

        TemporarySymbolEntry* mid_se = new TemporarySymbolEntry(now_type, SymbolTable::getLabel());
        Operand *change_op = new SimpleOperand(mid_se);
        new LoadInstruction(change_op, addr, bb);
        addr = change_op;
    }
    ofnode->getSize()->genCode();
    new GetPtrInstruction(last_op, addr, ofnode->getSize()->getOperand(), new PointerType(now_type), bb);
    ofnode = (ArraySize*)(ofnode->getPrev());
    while(ofnode != nullptr) {
        // int cur_size = ofnode->getSize()->ComputeValue();
        now_type = (ArrayType*)(now_type->getItemType());
        // now_type->setItemCount(cur_size);
        TemporarySymbolEntry* temp_se = new TemporarySymbolEntry(new PointerType(now_type->getItemType()), SymbolTable::getLabel());
        Operand *new_op = new SimpleOperand(temp_se);
        ofnode->getSize()->genCode();
        new GetPtrInstruction(new_op, last_op, ofnode->getSize()->getOperand(), new PointerType(now_type), bb);
        last_op = new_op;
        ofnode = (ArraySize*)(ofnode->getPrev());
    }
    dst = last_op;

}

int BinaryExpr::ComputeValue(){
    int src1 = expr1->ComputeValue();
    int src2 = expr2->ComputeValue();
    int res = 0;
    switch (op)
    {
    case ADD:
        res = src1 + src2;
        break;
    case SUB:
        res = src1 - src2;
        break;
    case MULT:
        res = src1 * src2;
        break;
    case DIV:
        res = src1 / src2;
        break;
    case MOD:
        res = src1 % src2;
        break;
    default:
        std::cout << "othercase! in Binary cpt" << std::endl;
        break;
    }
    return res;
}

int UnaryExpr::ComputeValue(){
    int src1 = expr1->ComputeValue();
    int res = 0;
    switch (op)
    {
    case PAREN:
        res = src1;
        break;
    case MINUS:
        res = -src1;
        break;
    case PLUS:
        res = src1;
        break;
    default:
        std::cout << "othercase! in Unary cpt" << std::endl;
        break;
    }
    return res;
}

int Cond::ComputeValue(){
    std::cout << "othercase! in Cond cpt" << std::endl;
    return 0;
}

int Constant::ComputeValue(){
    int res = 1;
    if(this->getSymPtr()->isConstant()) {
        ConstantSymbolEntry* con_se = (ConstantSymbolEntry*)this->getSymPtr();
        res = con_se->getValue();
    }
    return res;
}

int Id::ComputeValue(){
    int res;
    if(this->getSymPtr()->isVariable()) {
        IdentifierSymbolEntry* id_se = (IdentifierSymbolEntry*)this->getSymPtr();
        if(id_se->isConst()){
            res = id_se->getConstValue();
            return res;
        }
        std::cout << "not const" << std::endl;
    }
    std::cout << "not vib" << std::endl;
    return 0;
}

int ArrayId::ComputeValue(){
    std::cout << "othercase! in ArrayId cpt" << std::endl;
    return 0;
}

int FuncCall::ComputeValue(){
    std::cout << "othercase! in FuncCall cpt" << std::endl;
    return 0;
}

int SysFuncCall::ComputeValue(){
    std::cout << "othercase! in SysFuncCall cpt" << std::endl;
    return 0;
}