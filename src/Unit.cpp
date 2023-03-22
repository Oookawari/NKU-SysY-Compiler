#include "Unit.h"
extern FILE *yyout;

void Unit::insertFunc(Function *f)
{
    func_list.push_back(f);
}

void Unit::removeFunc(Function *func)
{
    func_list.erase(std::find(func_list.begin(), func_list.end(), func));
}

void Unit::output() const
{
    fprintf(yyout, "declare void @putint(i32)\n");
    fprintf(yyout, "declare i32 @getint()\n");
    fprintf(yyout, "declare void @putch(i32)\n");
    for (auto i = global_head->getNext(); i != global_head; i = i->getNext())
        i->output();

    for (auto &func : func_list)
        func->output();
}

Unit::~Unit()
{
    Instruction *inst;
    inst = global_head->getNext();
    while (inst != global_head)
    {
        Instruction *t;
        t = inst;
        inst = inst->getNext();
        delete t;
    }

    for(auto &func:func_list)
        delete func;
}

void Unit::insertFollow(Instruction *src)
{
    
    src->setNext(global_head);

    src->setPrev(global_head->getPrev());
    global_head->getPrev()->setNext(src);
    global_head->setPrev(src);
}

void Unit::insert_init_Follow(Instruction *src)
{
    src->setNext(global_init);
    src->setPrev(global_init->getPrev());
    global_init->getPrev()->setNext(src);
    global_init->setPrev(src);
}

void Unit::genMachineCode(MachineUnit* munit) 
{
    AsmBuilder* builder = new AsmBuilder();
    builder->setUnit(munit);
    for (auto i = global_head->getNext(); i != global_head; i = i->getNext())
        i->genMachineCode(builder);
    for (auto &func : func_list)
        func->genMachineCode(builder);
}

void Unit::setMainFunc(Function *func){
    this->mainFunc = func;
}