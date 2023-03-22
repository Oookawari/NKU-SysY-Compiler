%option noyywrap
%{
    #define YY_NO_UNPUT
    #define YY_NO_INPUT

    #include "parser.h"
    #include <string.h>
    #include <ostream>
    #include <fstream>
    using namespace std;

    extern FILE *yyin; 
    extern FILE *yyout;
    extern bool dump_tokens;

    void DEBUG_FOR_LAB4(std::string s){
        std::string DEBUG_INFO = "[DEBUG LAB4]: \t" + s + "\n";
        fputs(DEBUG_INFO.c_str(), yyout);
    }

     // to judge if it's a single alphabet word
    int IS_ALPA(char c) {
    	if ('a' <= c && c <= 'z') {
    		return 1;
    	}
    	else if ('A' <= c && c <= 'Z') {
    		return 2;
    	}
    	return 0;
    }

    // init_num is the initial number of lines and words[used by counter]
    #define init_num 1
    int lines = init_num, words = init_num;

    string mark_lw (string inp, char* yytext, bool is_id = false, unsigned int pointer = 0) {
        string s = inp;
        if(pointer == 0){
    	    s = s + "\t" + std::to_string(lines) + "\t" + std::to_string(words);
    	    words += strlen(yytext);
        }
        else{
    	    s = s + "\t" + std::to_string(lines) + "\t" + std::to_string(words) + "\t" + std::to_string(pointer);
    	    words += strlen(yytext);
        }
        return s;
    }

    // define a precessor to print debug information
    #define PROC(para) string LEX_STR = para; LEX_STR = mark_lw(LEX_STR, yytext); DEBUG_FOR_LAB4(LEX_STR)
%}

FLOAT_HEX (0x(([1-9a-fA-F][0-9a-fA-F]*|0)\.[0-9a-fA-F]+))
FLOAT_OCT (0(([1-7][0-7]*|0)\.[0-7]+))

DECIMIAL ([1-9][0-9]*|0)
DECIMAL_FLOAT (([1-9][0-9]*|0)\.[0-9]+)
DECIMIAL_OCT (0([1-7][0-7]*|0))
DECIMIAL_HEX (0x([1-9a-fA-F][0-9a-fA-F]*|0))

ID [[:alpha:]_][[:alpha:][:digit:]_]*

COMMENTBEGIN "/*"
COMMENTELEMENT .
COMMENTEOL \n
COMMENTEND "*/"
LINECOMMENT \/\/[^\n]*

EOL (\r\n|\n|\r)
WHITE [\t ]

%x BLOCKCOMMENT

%%
    /*行注释*/
{LINECOMMENT} {
    yylineno++;
    lines++;
	words = init_num;
}
    /*块注释开始*/
{COMMENTBEGIN} {
    BEGIN BLOCKCOMMENT;
    words += 2;
}
    /*块注释结束*/
<BLOCKCOMMENT>{COMMENTEND} {
    words += 2;
    BEGIN INITIAL;
}
    /*块注释换行*/
<BLOCKCOMMENT>{COMMENTEOL} {
    yylineno++;
    lines++;
	words = init_num;
}
    /*块注释换行*/
<BLOCKCOMMENT>{COMMENTELEMENT} {
    words += strlen(yytext);
}
    /*SysY运行时函数*/
"getint" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:getint\tgetint");
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    return LIB_GETINT;
}
    /*SysY运行时函数*/
"getch" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:getch\tgetch");
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    return LIB_GETCH;
}
    /*SysY运行时函数*/
"getarray" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:getarray\tgetarray");
    return LIB_GETARRAY;
}
    /*SysY运行时函数*/
"getfloat" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:getfloat\tgetfloat");
    return LIB_GETFLOAT;
}
    /*SysY运行时函数*/
"getfarray" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:getfarray\tgetfarray");
    return LIB_GETFARRAY;
}
    /*SysY运行时函数*/
"putint" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:putint\tputint");
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    return LIB_PUTINT;
}
    /*SysY运行时函数*/
"putch" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:putch\tputch");
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    return LIB_PUTCH;
}
    /*SysY运行时函数*/
"putarray" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:putarray\tputarray");
    return LIB_PUTARRAY;
}
    /*SysY运行时函数*/
"putfloat" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:putfloat\tputfloat");
    return LIB_PUTFLOAT;
}
    /*SysY运行时函数*/
"putfarray" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:putfarray\tputfarray");
    return LIB_PUTFARRAY;
}
    /*SysY运行时函数*/
"putf" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("Sysy:putf\tputf");
    return LIB_PUTF;
}
    /*类型：int*/
"int" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("INT\tint");
    return INT;
}
    /*类型：float*/
"float" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("FLOAT\tfloat");
    return FLOAT;
}
    /*类型：bool*/
"bool" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("BOOL\tbool");
    return BOOL;
}
    /*类型：void*/
"void" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("VOID\tvoid");
    return VOID;
}
    /*关键字:if*/
"if" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("IF\tif");
    return IF;
};
    /*关键字:else*/
"else" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("ELSE\telse");
    return ELSE;
};
    /*关键字:break*/
"break" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("BREAK\tbreak");
    return BREAK;
}
    /*关键字:continue*/
"continue" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("CONTINUE\tcontinue");
    return CONTINUE;
}
    /*关键字:return*/
"return" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("RETURN\treturn");
    return RETURN;
}
    /*关键字:const*/
"const" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("CONST\tconst");
    return CONST;
}
    /*关键字:while*/
"while" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("WHILE\twhile");
    return WHILE;
}
    /*运算符:==*/
"==" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("EQUAL\t==");
    return EQUAL;
}
    /*运算符:!=*/
"!=" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("NOTEQUAL\t!=");
    return NOTEQUAL;
}
    /*运算符:=*/
"=" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("ASSIGN\t=");
    return ASSIGN;
}
    /*运算符:<=*/
"<=" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("LESSEQUAL\t<=");
    return LESSEQUAL;
}
    /*运算符:>=*/
">=" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("MOREEQUAL\t>=");
    return MOREEQUAL;
}
    /*运算符:<*/
"<" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("LESS\t<");
    return LESS;
}
    /*运算符:>*/
">" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("MORE\t>");
    return MORE;
}
    /*运算符:+*/
"+" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("ADD\t+");
    return ADD;
}
    /*运算符:-*/
"-" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("SUB\t-");
    return SUB;
}
    /*运算符:**/
"*" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("MULT\t*");
    return MULT;
}
    /*运算符:/*/
"/" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("DIV\t/");
    return DIV;
}
    /*运算符:%*/
"%" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("MOD\t%");
    return MOD;
}
    /*运算符:|*/
"|" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("OR\t|");
    return OR;
}
    /*运算符:&*/
"&" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("AND\t&");
    return AND;
}
    /*运算符:||*/
"||" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("LOR\t||");
    return LOR;
}
    /*运算符:&&*/
"&&" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("LAND\t&&");
    return LAND;
}
    /*运算符:!*/
"!" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("NOT\t!");
    return NOT;
}
    /*运算符:;*/
";" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("SEMICOLON\t;");
    return SEMICOLON;
}
    /*运算符:(*/
"(" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("LPAREN\t(");
    return LPAREN;
}
    /*运算符:)*/
")" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("RPAREN\t)");
    return RPAREN;
}
    /*符号:[*/
"[" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("LBRACK\t{");
    return LBRACK;
}
    /*符号:]*/
"]" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("RBRACK\t{");
    return RBRACK;
}
    /*符号:{*/
"{" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("LBRACE\t{");
    return LBRACE;
}
    /*符号:}*/
"}" {
    if(dump_tokens)
        DEBUG_FOR_LAB4("RBRACE\t}");
    return RBRACE;
}
    /*符号:,*/
"," {
    if(dump_tokens)
        DEBUG_FOR_LAB4("COMMA\t,");
    return COMMA;
}
    /*标识符*/
{ID} {
    if(dump_tokens)
        DEBUG_FOR_LAB4(yytext);
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    return ID;
}
    /*浮点数（16进制）*/
{FLOAT_HEX} {
    string s = "FLOAT_HEX\t";
    string text = "";
    text += yytext;
    text = text.substr(2);
    string integer_part;
    string fractional_part;

    for(int i = 0; i < (int)text.size(); i++){
        if(text[i] == '.') {
            integer_part = text.substr(0, i);
            fractional_part = text.substr(i + 1);
        }
    }

    float inte_hexnum = 0, frac_hexnum = 0;
    for(int i = 0; i < (int)integer_part.size(); i++){
        char temp = integer_part[i];
        if (IS_ALPA(temp) == 1) {
            inte_hexnum = inte_hexnum * 16 + 10 + temp - 'a';
        }
        else if (IS_ALPA(temp) == 2) {
            inte_hexnum = inte_hexnum * 16 + 10 + temp - 'A';
        }
        else {
            inte_hexnum = inte_hexnum * 16 + temp - '0';
        }
    }
    for(int i = (int)fractional_part.size() - 1; i >= 0; i--){
        char temp = fractional_part[i];
        if (IS_ALPA(temp) == 1) {
            frac_hexnum = frac_hexnum / 16 + 10 + temp - 'a';
        }
        else if (IS_ALPA(temp) == 2) {
            frac_hexnum = frac_hexnum / 16 + 10 + temp - 'A';
        }
        else {
            frac_hexnum = frac_hexnum / 16 + temp - '0';
        }
    }
    frac_hexnum /= 16;

    float hexnum = inte_hexnum + frac_hexnum;

    s += std::to_string(hexnum);
    s = mark_lw(s, yytext);
    if(dump_tokens)
        DEBUG_FOR_LAB4(s);
    yylval.floattype = hexnum;
    return FLOATNUM;
}

    /*浮点数（8进制）*/
{FLOAT_OCT} {
    string s = "FLOAT_OCT\t";
    string text = "";
    text += yytext;
    text = text.substr(1);
    string integer_part;
    string fractional_part;
    for(int i = 0; i < (int)text.size(); i++){
        if(text[i] == '.') {
            integer_part = text.substr(0, i);
            fractional_part = text.substr(i + 1);
        }
    }

    float inte_octnum = 0, frac_octnum = 0;
    for(int i = 0; i < (int)integer_part.size(); i++){
        char temp = integer_part[i];
        inte_octnum = inte_octnum * 8 + temp - '0';
    }
    for(int i = (int)fractional_part.size() - 1; i >= 0; i--){
        char temp = fractional_part[i];
        frac_octnum = frac_octnum / 8 + temp - '0';
    }
    frac_octnum /= 8;

    float octnum = inte_octnum + frac_octnum;

    s += std::to_string(octnum);
    s = mark_lw(s, yytext);
    DEBUG_FOR_LAB4(s);
    yylval.floattype = octnum;
    return FLOATNUM;
}
    /*浮点数（10进制）*/
{DECIMAL_FLOAT} {
    if(dump_tokens)
        DEBUG_FOR_LAB4(yytext);
    yylval.floattype = atof(yytext);
    return FLOATNUM;
}
    /*整数（16进制）*/
{DECIMIAL_HEX} {
    string s = "DECIMIAL_HEX\t";
    string text = "";
    text += yytext;
    text = text.substr(2);
    int hexnum = 0;
    for(int i = 0; i < (int)text.size(); i++){
        char temp = text[i];
        if (IS_ALPA(temp) == 1) {
            hexnum = hexnum * 16 + 10 + temp - 'a';
        }
        else if (IS_ALPA(temp) == 2) {
            hexnum = hexnum * 16 + 10 + temp - 'A';
        }
        else {
            hexnum = hexnum * 16 + temp - '0';
        }
    }
    s += std::to_string(hexnum);
    
    s = mark_lw(s, yytext);
    if(dump_tokens)
        DEBUG_FOR_LAB4(s);
    yylval.itype = hexnum;
    return INTEGER;
}
    /*整数（8进制）*/
{DECIMIAL_OCT} {
    string s = "DECIMIAL_OCT\t";
    string text = "";
    text += yytext;
    text = text.substr(1);
    int octnum = 0;
    for(int i = 0; i < (int)text.size(); i++){
        char temp = text[i];
        octnum = octnum * 8 + temp - '0';
    }
    s += std::to_string(octnum);
    
    s = mark_lw(s, yytext);
    if(dump_tokens)
        DEBUG_FOR_LAB4(s);
    yylval.itype = octnum;
    return INTEGER;
}
    /*整数（10进制）*/
{DECIMIAL} {
    if(dump_tokens)
        DEBUG_FOR_LAB4(yytext);
    yylval.itype = atoi(yytext);
    return INTEGER;
}
    /*end of line*/
{EOL} {
    yylineno++;
    lines++;
	words = init_num;
}
    /*制表符*/
{WHITE} {
    words++;
}
%%