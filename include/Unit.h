#ifndef __UNIT_H__
#define __UNIT_H__

#include <vector>
#include "Function.h"
#include "AsmBuilder.h"
#include <iostream>
class Unit
{
    typedef std::vector<Function *>::iterator iterator;
    typedef std::vector<Function *>::reverse_iterator reverse_iterator;

private:
    std::vector<Function *> func_list;
    Instruction *global_init;
    Instruction *global_head;
    Function *mainFunc; 
public:
    bool global_collecting = false;
    Unit(){
        global_head = new DummyInstruction();
        global_init = new DummyInstruction();
    };
    ~Unit() ;
    void insertFollow(Instruction *);
    void insert_init_Follow(Instruction *);
    void insertFunc(Function *);
    void removeFunc(Function *);
    void output() const;
    void setMainFunc(Function *);
    Function* getMainFunc(){return mainFunc;};
    iterator begin() { return func_list.begin(); };
    iterator end() { return func_list.end(); };
    reverse_iterator rbegin() { return func_list.rbegin(); };
    reverse_iterator rend() { return func_list.rend(); };
    Instruction* getGlobal_init(){return global_init;}
    void genMachineCode(MachineUnit* munit);
};

extern Unit *unit_ptr;

#endif