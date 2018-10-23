%{
//  #include "lexParse.h"
  #include <string>
  using namespace std;

  #include "y.tab.h"
//  #include "lexParse.h" 
  #include<stdio.h>
//  #include <string>

//  using namespace std;

  int currLine = 1, currPos = 1;
 extern void yyerror( string msg );
 extern void yyerror (const char *error);

int isAlpha( char ch ){
  return ( ch>96 && ch <123 ) || ( ch>64 && ch <91 );
}
int addIdentifier(){
  if( !yyleng ){
    return -1;
  }
  if( !isAlpha( yytext[0] ) ){
    yyerror( "Identifier must begin with a letter" );
    return -1;
  }
  if( yytext[yyleng-1]=='_' ){
    yyerror( "Identifier cannot end with an underscore" );
    return -1;
  }
  yylval.text = new string(yytext);
  return IDENT;
}

%}

/* To present invalid varName as symbol error:
    varName               [a-zA-Z](([a-zA-Z]|[0-9]|[_])*([a-zA-Z]|[0-9]))* 
    To output specific error, accept any alphanumeric and test later 
*/
identifier	[a-zA-Z0-9_]+
dig		[0-9]+
space		[ \t]+
newline		"\n"
comment        	##(.)*"\n"
unknown		.

/*  Float regex catches ints, but intLit catches first  */

%%

"function"	{ currPos+=yyleng; return FUNCTION; }
"beginparams"	{ currPos+=yyleng; return BEGIN_PARAM; }
"endparams"	{ currPos+=yyleng; return END_PARAM; }
"beginlocals"	{ currPos+=yyleng; return BEGIN_LOCALS; }
"endlocals"	{ currPos+=yyleng; return END_LOCALS; }
"beginbody"	{ currPos+=yyleng; return BEGIN_BODY; }
"endbody"	{ currPos+=yyleng; return END_BODY; }
"integer"	{ currPos+=yyleng; return INTEGER; }
"array"		{ currPos+=yyleng; return ARRAY; }
"of"		{ currPos+=yyleng; return OF; }
"if"		{ currPos+=yyleng; return IF; }
"then"		{ currPos+=yyleng; return THEN; }
"endif"		{ currPos+=yyleng; return ENDIF; }
"else"		{ currPos+=yyleng; return ELSE; }
"while"		{ currPos+=yyleng; return WHILE; }
"do"		{ currPos+=yyleng; return DO; }
"beginloop"	{ currPos+=yyleng; return BEGINLOOP; }
"endloop"	{ currPos+=yyleng; return ENDLOOP; }
"continue"	{ currPos+=yyleng; return CONTINUE; }
"read"		{ currPos+=yyleng; return READ; }
"write"		{ currPos+=yyleng; return WRITE; }
"and"		{ currPos+=yyleng; return AND; }
"or"		{ currPos+=yyleng; return OR; }
"not"		{ currPos+=yyleng; return NOT; }
"true"		{ currPos+=yyleng; return TRUE; }
"false"		{ currPos+=yyleng; return FALSE; }
"return"	{ currPos+=yyleng; return RETURN; }
"-"		{ currPos+=yyleng; return SUB; }
"+"		{ currPos+=yyleng; return ADD; }
"*"		{ currPos+=yyleng; return MULT; }
"/"		{ currPos+=yyleng; return DIV; }
"%"		{ currPos+=yyleng; return MOD; }
"=="		{ currPos+=yyleng; return EQ; }
"<>"		{ currPos+=yyleng; return NEQ; }
"<"		{ currPos+=yyleng; return LT; }
">"		{ currPos+=yyleng; return GT; }
"<="		{ currPos+=yyleng; return LTE; }
">="		{ currPos+=yyleng; return GTE; }
";"		{ currPos+=yyleng; return SEMICOLON; }
":"		{ currPos+=yyleng; return COLON; }
","		{ currPos+=yyleng; return COMMA; }
"("		{ currPos+=yyleng; return L_PAREN; }
")"		{ currPos+=yyleng; return R_PAREN; }
"["		{ currPos+=yyleng; return L_SQ_BRACKET; }
"]"		{ currPos+=yyleng; return R_SQ_BRACKET; }
":="		{ currPos+=yyleng; return ASSIGN; }

{space}		{ currPos += yyleng; }
{comment}	{ currPos = 1; currLine++; }
{newline}	{ currPos = 1; currLine++; }

(\.{dig}+)|({dig}+(\.{dig}*)?([eE][+-]?[0-9]+)?)   {currPos += yyleng; yylval.text = new string(yytext); return NUMBER;}

{identifier}    { return addIdentifier(); }

{unknown}	{ yyerror( "Unknown symbol" ); }


%%


