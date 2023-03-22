#include "Operand.h"
#include <sstream>
#include <algorithm>
#include <string.h>
#include <iostream>
void Operand::removeUse(Instruction *inst)
{
    auto i = std::find(uses.begin(), uses.end(), inst);
    if(i != uses.end())
        uses.erase(i);
}
static ZeroOperand zero_op;
static TrueOperand true_op;
static Zeroinitializer init_op;
ZeroOperand *zero_oprand = &zero_op;
TrueOperand *true_oprand = &true_op;
Zeroinitializer *zeroinitializer = &init_op;