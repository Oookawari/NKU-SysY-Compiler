%code top{
    #include <iostream>
    #include <assert.h>
    #include "parser.h"
    extern Ast ast;
    int yylex();
    int yyerror( char const * );
}

%code requires {
    #include "Ast.h"
    #include "SymbolTable.h"
    #include "Type.h"
}

%union {
    int itype;
    char* strtype;
    float floattype;
    bool booltype;
    StmtNode* stmttype;
    ExprNode* exprtype;
    Type* type;
    Node* othertype;
}

%start Program
%token <strtype> ID 
%token <itype> INTEGER
%token <floattype> FLOATNUM
%token IF ELSE BREAK CONTINUE WHILE
%token INT FLOAT VOID BOOL
%token LPAREN RPAREN LBRACK RBRACK LBRACE RBRACE SEMICOLON COMMA
%token ADD SUB MULT DIV MOD LOR LAND LESS MORE LESSEQUAL MOREEQUAL ASSIGN EQUAL NOTEQUAL NOT AND OR
%token <strtype>LIB_GETINT LIB_PUTINT LIB_PUTCH
%token <strtype>LIB_GETCH LIB_GETARRAY LIB_GETFLOAT LIB_GETFARRAY  LIB_PUTARRAY LIB_PUTFLOAT LIB_PUTFARRAY LIB_PUTF
%token CONST
%token RETURN

%type <stmttype> Stmts Stmt AssignStmt BlockStmt IfStmt ReturnStmt DeclStmt FuncDef ConstDeclStmt WhileStmt ExpStmt BreakStmt ContinueStmt
%type <exprtype> LVal Exp Cond PrimaryExp ParenExp MinusExp MultiExp AddExp LCompExp LEqualExp LogicORExp LogicANDExp FuncCall
%type <type> Type
%type <othertype> InitAssign IDList ConstIDList FuncDefList ParamList ARRAYSIZE InitValue InitValueList ArrayParamSize

%precedence THEN
%precedence ELSE
%%
    /* 程序语法分析入口 */
Program
    : Stmts {
        ast.setRoot($1);
    }
    ;
    /* 语句序列 */
Stmts
    : Stmt {$$=$1;}
    | Stmts Stmt{
        $$ = new SeqNode($1, $2);
    }
    ;
    /* 语句 */
Stmt
    : AssignStmt {$$=$1;}
    | BlockStmt {$$=$1;}
    | IfStmt {$$=$1;}
    | ReturnStmt {$$=$1;}
    | DeclStmt {$$=$1;}
    | FuncDef {$$=$1;}
    | ConstDeclStmt {$$=$1;}
    | WhileStmt {$$=$1;}
    | ExpStmt {$$=$1;}
    | SEMICOLON {$$=new EmptyStmt();}
    | BreakStmt {$$=$1;}
    | ContinueStmt {$$=$1;}
    
    ;
    /* 左值 */
LVal
    : ID {
        SymbolEntry *se;
        se = identifiers->lookup($1);
        if(se == nullptr)
        {
            fprintf(stderr, "identifier \"%s\" is undefined\n", (char*)$1);
            delete [](char*)$1;
            assert(se != nullptr);
        }
        $$ = new Id(se);
        delete []$1;
    }
    |
    ID ARRAYSIZE {
        SymbolEntry *se;
        se = identifiers->lookup($1);
        if(se == nullptr)
        {
            fprintf(stderr, "identifier \"%s\" is undefined\n", (char*)$1);
            delete [](char*)$1;
            assert(se != nullptr);
        }
        $$ = new ArrayId(se, $2);
        delete []$1;
    }
    ;
    /* 赋值语句 */
AssignStmt
    :
    LVal ASSIGN Exp SEMICOLON {
        $$ = new AssignStmt($1, $3);
    }
    ;
    /* 块语句 */
BlockStmt
    :   LBRACE 
        {identifiers = new SymbolTable(identifiers);} 
        Stmts RBRACE 
        {
            $$ = new CompoundStmt($3);
            SymbolTable *top = identifiers;
            identifiers = identifiers->getPrev();
            delete top;
        }
        | LBRACE RBRACE
       {$$=new EmptyStmt();}

    ;
    /* 循环语句 */
WhileStmt
    :
    WHILE LPAREN Cond RPAREN Stmt {
        $$ = new WhileStmt($3, $5);
    }
    /* 条件语句 */
IfStmt
    : IF LPAREN Cond RPAREN Stmt %prec THEN {
        $$ = new IfStmt($3, $5);
    }
    | IF LPAREN Cond RPAREN Stmt ELSE Stmt {
        $$ = new IfElseStmt($3, $5, $7);
    }
    ;
    /* 返回语句 */
ReturnStmt
    :
    RETURN Exp SEMICOLON{
        $$ = new ReturnStmt($2);
    }
    ;
    /* 表达式 */
Exp
    :
    AddExp {$$ = $1;}
    ;
    /* Condition */
Cond
    :
    LogicORExp {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new Cond(se, $1);
        // $$ = $1;
    }
    ;
    /* 元表达式 */
PrimaryExp
    :
    LVal {
        $$ = $1;
    }
    | INTEGER {
        SymbolEntry *se = new ConstantSymbolEntry(TypeSystem::intType, $1);
        $$ = new Constant(se);
    }
    | FLOATNUM {
        SymbolEntry *se = new FloatConstantSymbolEntry(TypeSystem::floatType, $1);
        $$ = new Constant(se);
    }
    | FuncCall {
        $$ = $1;
    }
    ;
    /* 括号表达式 */
ParenExp 
    :
    PrimaryExp {$$ = $1;}
    |
    LPAREN LogicORExp RPAREN
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::PAREN, $2);
    }
    /* 单目运算表达式 */
MinusExp
    :
    ParenExp {$$ = $1;}
    |
    ADD MinusExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::PLUS, $2);
    }
    |
    SUB MinusExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::MINUS, $2);
    }
    |
    NOT MinusExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::NOT, $2);
    }
    ;
    /* 乘除运算表达式 */
MultiExp
    :
    MinusExp {$$ = $1;}
    |
    MultiExp MULT MinusExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MULT, $1, $3);
    }
    |
    MultiExp DIV MinusExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::DIV, $1, $3);
    }
    |
    MultiExp MOD MinusExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MOD, $1, $3);
    }
    ;
    /* 加减运算表达式 */
AddExp
    :
    MultiExp {$$ = $1;}
    |
    AddExp ADD MultiExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::ADD, $1, $3);
    }
    |
    AddExp SUB MultiExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::SUB, $1, $3);
    }
    ;
    /* 比较运算表达式*/

LCompExp
    :
    AddExp { $$ = $1;}
    |
    LCompExp LESS AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LESS, $1, $3);
    }
    |
    LCompExp MORE AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MORE, $1, $3);
    }
    |
    LCompExp LESSEQUAL AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LESSEQUAL, $1, $3);
    }
    |
    LCompExp MOREEQUAL AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MOREEQUAL, $1, $3);
    }
    ;
    /* 相等运算表达式*/
LEqualExp
    :
    LCompExp { $$ = $1;}
    |
    LEqualExp EQUAL LCompExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::EQUAL, $1, $3);
    }
    |
    LEqualExp NOTEQUAL LCompExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::NOTEQUAL, $1, $3);
    }
    ;
    /* 逻辑or运算表达式 */
LogicORExp
    :
    LogicANDExp { $$ = $1;}
    |
    LogicORExp LOR LogicANDExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LOR, $1, $3);
    }
    ;
    /* logic and expr */
LogicANDExp
    :
    LEqualExp { $$ = $1;}
    |
    LogicANDExp LAND LEqualExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LAND, $1, $3);
    }
    ;
    /* 类型 */
Type
    : INT {
        $$ = TypeSystem::intType;
    }
    | VOID {
        $$ = TypeSystem::voidType;
    }
    | FLOAT {
        $$ = TypeSystem::floatType;
    }
    ;
InitValueList
    :
    InitValue {
        $$ = new ArrayInit($1);
    }
    |
    InitValue COMMA InitValueList {
        $$ = new ArrayInit($1, $3);
    }
    ;
InitValue
    :
    Exp {
        $$ = $1;
    }
    |
    LBRACE InitValueList RBRACE {
        $$ = $2;
    }
    |
    LBRACE RBRACE {
        $$ = nullptr;
    }
    ;
    /* 赋值 */
InitAssign 
    :
    ID ASSIGN Exp {
        $$ = new InitAssign($1, $3);
    }
    |
    ID ARRAYSIZE ASSIGN InitValue {
        $$ = new InitArrayAssign($1, $2, $4);
    }
    ;
    /* the size when we define an array */
ARRAYSIZE
    :
    LBRACK Exp RBRACK ARRAYSIZE {
        $$ = new ArraySize($2, $4);
    }
    |
    LBRACK Exp RBRACK {
        $$ = new ArraySize($2);
    }
    ;
    /* 变量声明 标识符列表 */
IDList
    :
    InitAssign COMMA IDList {
        $$ = new IDList($1, $3);
    }
    |
    ID COMMA IDList {
        Node* temp = new InitID($1);
        $$ = new IDList(temp, $3);
        delete []$1;
    }
    |
    ID ARRAYSIZE COMMA IDList {
        Node* temp = new InitID($1, $2);
        $$ = new IDList(temp, $4);
        delete []$1;
    }
    |
    InitAssign {
        $$ = new IDList($1);
    }
    |
    ID {
        Node* temp = new InitID($1);
        $$ = new IDList(temp);
        delete []$1;
    }
    |
    ID ARRAYSIZE {
        // Array
        Node* temp = new InitID($1, $2);
        $$ = new IDList(temp);
        delete []$1;
    }
    ;
    /* 常量声明 标识符列表*/
ConstIDList
    :
    InitAssign COMMA ConstIDList {
        $$ = new IDList($1, $3);
    }
    |
    InitAssign {
        $$ = new IDList($1);
    }
    ;
    /* 表达式语句 */
ExpStmt
    :
    Exp SEMICOLON {
        $$ = new ExpStmt($1);
    }
    ;
    /* 变量声明语句 */
DeclStmt
    :
    Type IDList SEMICOLON {
        $$ = new DeclStmt($2);
        IDList* temp = (IDList*)$2;
        while(temp != nullptr){
            SymbolEntry *se;
            InitID* ids = (InitID*)temp->getId();
            char* names = ids->getName();
            /*检查：是否重复声明*/
            if(identifiers->lookupLocalVariable(names)) {
                fprintf(stderr, "Variable %s has been declared!", names);
            }
            if (ids->getArraySize() != nullptr) {
                Type* aryType = new ArrayType($1);
                se = new IdentifierSymbolEntry(aryType, names, identifiers->getLevel());
                // Is array
                ((IdentifierSymbolEntry *)se)->setAsArray();
            }
            else {
                se = new IdentifierSymbolEntry($1, names, identifiers->getLevel());
            }
            ids->SetSymPtr(se);
            identifiers->install(names, se);
            temp = (IDList*)temp->getPrev();
        }
    }
    ;
    /* 常量声明语句 */
ConstDeclStmt
    :
    CONST Type ConstIDList SEMICOLON {
        $$ = new ConstDeclStmt($3);
        IDList* temp = (IDList*)$3;
        while(temp != nullptr){
            SymbolEntry *se;
            InitID* ids = (InitID*)temp->getId();
            char* names = ids->getName();
            /*检查：是否重复声明*/
            if(identifiers->lookupLocalVariable(names)) {
                fprintf(stderr, "Variable %s has been declared!", names);
            }
            se = new IdentifierSymbolEntry($2, names, identifiers->getLevel(),true);
            ids->SetSymPtr(se);
            identifiers->install(names, se);
            temp = (IDList*)temp->getPrev();
        }
    }
    ;
ArrayParamSize
    :
    LBRACK Exp RBRACK ArrayParamSize {
        $$ = new ArraySize($2, $4);
    }
    |
    %empty {
        $$ = nullptr;
    }
    ;
    /* 常量声明语句 */
FuncDefList
    :
    Type ID COMMA FuncDefList {
        Node* temp = new InitID($2);
        $$ = new IDList(temp, $4);
        SymbolEntry *se;
        char* names = ((InitID*)temp)->getName();
        se = new IdentifierSymbolEntry($1, names, identifiers->getLevel());
        identifiers->install(names, se);
        ((InitID*)temp)->SetSymPtr(se);
        delete []$2;
    }
    |
    Type ID {
        Node* temp = new InitID($2);
        $$ = new IDList(temp);
        SymbolEntry *se;
        char* names = ((InitID*)temp)->getName();
        se = new IdentifierSymbolEntry($1, names, identifiers->getLevel());
        identifiers->install(names, se);
        ((InitID*)temp)->SetSymPtr(se);
        delete []$2;
    }
    |
    Type ID LBRACK RBRACK ArrayParamSize {
        Node* temp = new InitID($2, $5);
        $$ = new IDList(temp);
        SymbolEntry *se;
        char* names = ((InitID*)temp)->getName();
        ArrayType *aryType = new ArrayType($1);
        se = new IdentifierSymbolEntry(aryType, names, identifiers->getLevel());
        identifiers->install(names, se);
        ((InitID*)temp)->SetSymPtr(se);
        delete []$2;
    }
    |
    Type ID LBRACK RBRACK ArrayParamSize COMMA FuncDefList {
        Node* temp = new InitID($2, $5);
        $$ = new IDList(temp, $7);
        SymbolEntry *se;
        char* names = ((InitID*)temp)->getName();
        ArrayType *aryType = new ArrayType($1);
        se = new IdentifierSymbolEntry(aryType, names, identifiers->getLevel());
        identifiers->install(names, se);
        ((InitID*)temp)->SetSymPtr(se);
        delete []$2;
    }
    |
    %empty {
        $$ = nullptr;
    }
    ;
    /* 函数声明 */
FuncDef
    :
    Type ID LPAREN {
        Type *funcType;
        funcType = new FunctionType($1,{},{});
        SymbolEntry *se = new IdentifierSymbolEntry(funcType, $2, identifiers->getLevel());
        ((IdentifierSymbolEntry *)se)->setAsFunc();
        identifiers->install($2, se);
        identifiers = new SymbolTable(identifiers);
    } FuncDefList RPAREN {
        SymbolEntry *func_se = identifiers->lookup($2);
        FunctionType* temp_type = (FunctionType*)func_se->getType();
        IDList* temp_list = (IDList*)$5;
        while(temp_list != nullptr) {
            //获取参数id
            InitID* temp_id = (InitID*)temp_list->getId();
            //获取参数se
            IdentifierSymbolEntry *temp_se = (IdentifierSymbolEntry*)identifiers->lookup(temp_id->getName());
            //由se获取type并加入vector
            // temp_type->getParamsType().push_back(temp_se->getType());
            // temp_type->getParamsSe().push_back(temp_se);
            temp_type->push_param_type(temp_se->getType());
            temp_type->push_param_se(temp_se);
            temp_list = (IDList*)temp_list->getPrev();
        }
    } BlockStmt
    {
        SymbolEntry *func_se = identifiers->getPrev()->lookup($2);
        $$ = new FunctionDef(func_se, $5, $8);
        SymbolTable *top = identifiers;
        identifiers = identifiers->getPrev();
        delete top;
        delete []$2;
    }
    ;
    /* 函数调用 参数列表 */
ParamList
    :
    Exp COMMA ParamList {
        $$ = new ParamList($1, $3);
    }
    |
    Exp{
        $$ = new ParamList($1);
    }
    ;
    /*continue*/
ContinueStmt
    :
    CONTINUE SEMICOLON {
        $$ = new ContinueStmt();
    }
    ;
    /*break*/
BreakStmt
    :
    BREAK SEMICOLON {
        $$ = new BreakStmt();
    }
    ;
    /* 函数调用 */
FuncCall
    :
    ID LPAREN RPAREN {
        SymbolEntry *FuncSe;
        //FuncSe = identifiers->lookupGlobalVariable($1);
        FuncSe = identifiers->lookup($1);
        if(FuncSe == nullptr)
        {
            fprintf(stderr, "identifier \"%s\" is undefined\n", (char*)$1);
            delete [](char*)$1;
            assert(FuncSe != nullptr);
        }
        Type *retType;
        retType = ((FunctionType *)(((IdentifierSymbolEntry *)FuncSe)->getType()))->getRetType();
        SymbolEntry *ret = new TemporarySymbolEntry(retType, SymbolTable::getLabel());
        // SymbolEntry *ret = new TemporarySymbolEntry(new IntType(32), SymbolTable::getLabel());
        $$ = new FuncCall(ret, FuncSe);
        delete []$1;
    }
    |
    ID LPAREN ParamList RPAREN {
        SymbolEntry *FuncSe;
        //FuncSe = identifiers->lookupGlobalVariable($1);
        FuncSe = identifiers->lookup($1);
        if(FuncSe == nullptr)
        {
            fprintf(stderr, "identifier \"%s\" is undefined\n", (char*)$1);
            delete [](char*)$1;
            assert(FuncSe != nullptr);
        }
        Type *retType;
        retType = ((FunctionType*)((IdentifierSymbolEntry*)FuncSe->getType()))->getRetType();
        SymbolEntry *ret = new TemporarySymbolEntry(retType, SymbolTable::getLabel());
        // SymbolEntry *ret = new TemporarySymbolEntry(new IntType(32), SymbolTable::getLabel());
        $$ = new FuncCall(ret, FuncSe, $3);
        delete []$1;
    }
    |
    LIB_PUTINT LPAREN ParamList RPAREN {
        Type *funcType;
        funcType = new FunctionType(TypeSystem::voidType,{},{});
        SymbolEntry *se = new IdentifierSymbolEntry(funcType, $1, identifiers->getLevel());
        SymbolEntry *ret = new TemporarySymbolEntry(TypeSystem::voidType, SymbolTable::getLabel());
        $$ = new SysFuncCall(ret, $3,SysFuncCall::PUTINT);
    }
    |
    LIB_GETINT LPAREN RPAREN {
        Type *funcType;
        funcType = new FunctionType(TypeSystem::intType,{},{});
        SymbolEntry *se = new IdentifierSymbolEntry(funcType, $1, identifiers->getLevel());
        SymbolEntry *ret = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new SysFuncCall(ret, SysFuncCall::GETINT);
    }
    |
    LIB_PUTCH LPAREN ParamList RPAREN {
        Type *funcType;
        funcType = new FunctionType(TypeSystem::voidType,{},{});
        SymbolEntry *se = new IdentifierSymbolEntry(funcType, $1, identifiers->getLevel());
        SymbolEntry *ret = new TemporarySymbolEntry(TypeSystem::voidType, SymbolTable::getLabel());
        $$ = new SysFuncCall(ret, $3, SysFuncCall::PUTCH);
    }
    ;

%%

int yyerror(char const* message)
{
    std::cerr<<message<<std::endl;
    return -1;
}