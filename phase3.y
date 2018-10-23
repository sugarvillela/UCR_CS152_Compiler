/* bison */
%{
	#include "lexParse.h"
	#include <string>
	using namespace std;
%}
%union{
  string *text;
}

%error-verbose

%token FUNCTION 		/* 'function' */
%token <text> NUMBER	/* literal */
%token <text> IDENT		/* variable name */
%token ASSIGN
%token BEGIN_PARAM END_PARAM 
%token BEGIN_LOCALS END_LOCALS 
%token BEGIN_BODY END_BODY
%token INTEGER ARRAY OF		/* 'integer' */
%token IF THEN ENDIF ELSE
%token WHILE DO BEGINLOOP ENDLOOP CONTINUE 
%token READ WRITE
%token TRUE FALSE
%token RETURN 
%token SEMICOLON COLON COMMA
%token R_PAREN L_SQ_BRACKET R_SQ_BRACKET
%token EQ
%token NEQ LT GT LTE GTE
%token AND OR NOT 
%left  SUB ADD 
%left  MULT DIV MOD
%token L_PAREN
%nonassoc UMINUS

%% 
prog:	%empty					{ dev::dr( "prog->empty" ); }
	| funct prog				{ 
									dev::dr( "prog->funct" ); 
									errOn.badMain();							/* End of parse: check 'main' function */
								}
	;
/* Action triggers */
funct_id: FUNCTION ident		{ dev::dr( "funct_id->FUNCTION ident" );
								  flow.push("funct");							/* push function handler */
								  flow.start( $<text>2 );						/* funct_h::start */
								}
	;
param: 	BEGIN_PARAM				{ 
									dev::dr( "param->BEGIN_PARAM" ); 
									chex.inParam=true;
									vals.push("begin_param"); 					/* push param handler; ident calls param_h::start */
								}
	;
end_param: 	END_PARAM			{ 
									dev::dr( "end_param->END_PARAM" );
									chex.inParam=false;
									vals.finish();								/* finalize list */
									vals.pop();									/* pop param handler */
								}
	;
begin_locals:	BEGIN_LOCALS	{
									dev::dr( "begin_locals->BEGIN_LOCALS" );
									vals.push("begin_local");					/* push local handler */
								}
	;
end_locals:	END_LOCALS			{ 
									dev::dr( "end_locals->END_LOCALS" );
									vals.pop();									/* pop local handler */
								}
	;
while: WHILE					{ 
									dev::dr( "while->WHILE" );
									if(!chex.inDoWhile)							/* only true after 'endloop' and before 'while' */
									{						
										flow.push("while");						/* push while handler */
									}
								}
	;
do:  DO							{ 
									dev::dr( "do->DO" ); 
									flow.push("do_while");						/* push do while handler */
								}
	;
wLoopTop: boolexp BEGINLOOP		{
									dev::dr( "wLoopTop->boolexp BEGINLOOP" ); 
									flow.start( $<text>1 );						/* while_h::start */
								}
	;
endloop:   ENDLOOP				{ 
									dev::dr( "endloop->ENDLOOP" );
									flow.finish();								/* finish while loop */
								}
	;
beginif:  IF					{
									dev::dr( "beginif->IF" );
									flow.push("if");							/* push if else handler */
								}
	;
then: boolexp THEN				{ 
									dev::dr( "then->boolexp THEN" ); 
									flow.start( $<text>1 );						/* if_h::start */
								}
	;
beginelse:   ELSE				{
									dev::dr( "beginelse->ELSE" );
									flow.keyword_else();						/* Ignore, or labels handled by if_h */
								}
	;
funct:	funct_id SEMICOLON param decl_list end_param begin_locals decl_list end_locals BEGIN_BODY stmt_list END_BODY
     	{ 
     		dev::dr( "funct->funct_id SEMICOLON param decl_list end_param begin_locals decl_list end_locals BEGIN_BODY stmt_list END_BODY" );
     		flow.finish();														/* finalize function */
     		flow.pop();															/* pop function handler */
     	}
	;
decl: 	 ident_list COLON INTEGER SEMICOLON		{ 
													dev::dr( "decl->ident_list COLON INTEGER SEMICOLON" );
													vals.finish();				/* finalize a var list */
												}
    	| ident_list COLON ARRAY L_SQ_BRACKET 
		  number R_SQ_BRACKET OF INTEGER SEMICOLON	
												{ 
													dev::dr( "decl->ident_list COLON ARRAY L_SQ_BRACKET number R_SQ_BRACKET SEMICOLON" ); 
													vals.finish( $<text>5 );	/* finalize an array list */
													errOn.arrayInParam();
												}
	;
decl_list: 
	 %empty										{ dev::dr( "decl_list->empty" ); }
	 | decl decl_list							{ dev::dr( "decl_list->decl decl_list" ); }

/* Always a list */
ident: 	 IDENT									{ dev::dr( "ident->IDENT", $1 );
													vals.start( $<text>1 );		/* add to list */
												}
     	;
ident_list:
	  ident										{ dev::dr( "ident_list->ident" ); }
	| ident COMMA ident_list					{ dev::dr( "ident_list->ident COMMA ident_list" ); }
	;
read: READ										{ 
													dev::dr( "read->READ" ); 
													vals.push("read");			/* push read handler */
												}
	;
write: WRITE									{ 
													dev::dr( "write->WRITE" ); 
													vals.push("write");			/* push write handler */
												}
	;
stmt: var ASSIGN exp SEMICOLON		{ 
										dev::dr( "stmt- var ASSIGN exp SEMICOLON" ); 
										gen.assign( $<text>1, $<text>3 );		/* write assign code */
									}
									
    | stmt_if SEMICOLON					{
    										dev::dr( "stmt->stmt_if" ); 
    										flow.finish();						/* finish if else */
											flow.pop();							/* pop if else handler */
    									}
	| while wLoopTop 
	  stmt_list endloop SEMICOLON		{ 
											dev::dr( "stmt-> while wLoopTop stmt_list endloop" );
											flow.pop();							/* pop while handler */
										}
	| do BEGINLOOP stmt_list
	  endloop WHILE boolexp	SEMICOLON	{ 
											dev::dr( "stmt->do beginloop stmt_list endloop WHILE boolexp SEMICOLON" ); 
											flow.finish($<text>6);				/* finish do while loop */
											flow.pop();							/* pop do while handler */
										}
	| read var_list SEMICOLON		{ 
										dev::dr( "stmt->read var_list SEMICOLON" ); 
										vals.finish();							/* finish read */
										vals.pop();								/* pop read handler */
									}
	| write var_list SEMICOLON		{
										dev::dr( "stmt->write var_list SEMICOLON" );
										vals.finish();							/* finish write */
										vals.pop();								/* pop write handler */
									}
	| CONTINUE SEMICOLON			{
										dev::dr( "stmt->CONTINUE SEMICOLON" );
										flow.keyword_continue();				/* Error, or labels handled by while_h */
									}
	| RETURN exp SEMICOLON			{
										dev::dr( "stmt->RETURN exp SEMICOLON" );
										chex.funReturns=true;
										gen.nop( $<text>2, "ret" );				/* code_gen::nop() */
									}
	| var ASSIGN relexp SEMICOLON	{											/* Not in diagram, but should be */
										dev::dr( "stmt->var ASSIGN relexp SEMICOLON" );				
										gen.assign( $<text>1, $<text>3 );		/* write assign code */
									}
	| error SEMICOLON				{ dev::dr( "stmt->ERROR!!!" ); }
	;
stmt_if: if ENDIF					{ dev::dr( "stmt_if->if ENDIF SEMICOLON" ); }
	| if else ENDIF					{ dev::dr( "stmt_if->if else ENDIF SEMICOLON" ); }
	;

if:	beginif then stmt_list			{ dev::dr( "if->beginif then stmt_list" ); }
	;

else:	beginelse stmt_list			{ dev::dr( "else->beginelse stmt_list" ); }
	;
stmt_list:
	 stmt						{ dev::dr( "stmt_list->stmt" ); }
	| stmt stmt_list			{ dev::dr( "stmt_list->stmt stmt_list" ); }
	;

boolexp: andexp					{ dev::dr( "boolexp-andexp>" ); }
	| andexp OR boolexp			{ dev::dr( "boolexp->andexp OR boolexp" ); }
	;
andexp:	relexp					{ dev::dr( "andexp->relexp" ); }
	| andexp AND relexp			{ dev::dr( "andexp->andexp AND relexp" ); }
	;
relexp:	
	  exp EQ exp				{ 
									dev::dr( "relexp->exp EQ exp" );
									$<text>$ = gen.AL( "==", $<text>1, $<text>3 );/* Gen code; new temp to $$*/	
								}
	| exp NEQ exp				{ 
									dev::dr( "relexp->exp NEQ exp" ); 
									$<text>$ = gen.AL( "!=", $<text>1, $<text>3 );/* Gen code; new temp to $$*/	
								}
	| exp LT exp				{ 
									dev::dr( "relexp->exp LT exp" ); 
									$<text>$ = gen.AL( "<", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
								}
	| exp GT exp				{ 
									dev::dr( "relexp->exp GT exp" ); 
									$<text>$ = gen.AL( ">", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
								}
	| exp LTE exp				{ 
									dev::dr( "relexp->exp LTE exp" ); 
									$<text>$ = gen.AL( "<=", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
								}
	| exp GTE exp				{ 
									dev::dr( "relexp->exp GTE exp" ); 
									$<text>$ = gen.AL( ">=", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
								}								
	| TRUE						{ 
									dev::dr( "relexp->TRUE" );
									$<text>$=vals.middle( new string("1") );				/* fBody::middle, temp to $$ */
								}
	| FALSE						{ 
									dev::dr( "relexp->FALSE" );
									$<text>$=vals.middle( new string("0") );				/* fBody::middle, temp to $$ */
								}
	| L_PAREN boolexp R_PAREN	{ 
									dev::dr( "relexp->L_PAREN boolexp R_PAREN" ); 
									$<text>$ = $<text>2;
								}
	| NOT relexp				{ 
									dev::dr( "relexp->NOT relexp" ); 
									sTable.negate( (*$<text>2) );
									$<text>$ = $<text>2;
								}
	;
exp: multexp					{ dev::dr( "exp->multexp" ); sTable.info($<text>1);}
	| multexp ADD exp			{ 
									dev::dr( "exp->multexp ADD multexp" ); 
									$<text>$ = gen.AL( "+", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
								}
	| multexp SUB exp			{ 
									dev::dr( "exp->multexp SUB multexp" ); 
									$<text>$ = gen.AL( "-", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
								}
	| SUB exp %prec UMINUS		{
									dev::dr( "exp->SUB exp %prec UMINUS" ); 
									sTable.complement( (*$<text>2) );
									$<text>$ = $<text>2;
								}
	;
multexp: term					{ dev::dr( "multexp->term" ); sTable.info($<text>1); }
	| term MULT term			{ 
									dev::dr( "multexp->term MULT term" ); 
									$<text>$ = gen.AL( "*", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
									
								}
	| term DIV term				{ 
									dev::dr( "multexp->term DIV term" ); 
									$<text>$ = gen.AL( "/", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
									
								}
	| term MOD term				{ 
									dev::dr( "multexp->term MOD term" ); 
									$<text>$ = gen.AL( "%", $<text>1, $<text>3 );/* Gen code; new temp to $$*/
									
								}//MINUS exp %prec UMINUS
	;

term:	var						{ dev::dr( "term->var" ); sTable.info($<text>1);}
    | number					{ dev::dr( "term->number" ); sTable.info($<text>1);}
	| L_PAREN exp R_PAREN		{ 
									dev::dr( "term->L_PAREN exp R_PAREN" ); sTable.info($<text>2);
									$<text>$ = $<text>2;
								}
	| IDENT L_PAREN exp R_PAREN	{ 
									dev::dr( "term->IDENT L_PAREN exp R_PAREN" );
									$<text>$=gen.fCall( $<text>1, $<text>3 );
								}
	;
number:	NUMBER					{ 
									dev::dr( "number->NUMBER", $1 );
									$<text>$=vals.number( $<text>1 );			/* fBody::number, temp to $$ */
								}
var:	ident					{ 
									dev::dr( "var->ident" ); 
									$<text>$=vals.middle( $<text>1 );			/* fBody::middle, temp to $$ */
								}
	| ident L_SQ_BRACKET exp R_SQ_BRACKET { 
									dev::dr( "var->ident L_SQ_BRACKET exp R_SQ_BRACKET" ); 
									$<text>$=vals.middle( $<text>1,$<text>3 );		/* fBody::middle, temp to $$ */
								}
	;
var_list: var					{ dev::dr( "var_list->var" ); }
	| var COMMA var_list		{ dev::dr( "var_list->var COMMA var_list" ); }
	;
%%

int main(int argc, char **argv) {
	dev::drCount=1;
	dev::dispReductions=	false;
	dev::dispErrors=		false;
	dev::dispConstructors=	false;
	dev::dispSymbolTable=	true;
	
   if (argc > 1) {
      yyin = fopen(argv[1], "r");
      if (yyin == NULL){
         printf("syntax: %s filename\n", argv[0]);
      }
   }
   yyparse();
   if( !chex.errState ){//Only emit code if no error
		cout << render.str() << endl;
   }
   return 0;
}
void yyerror(const char *msg) {
	//cout << render.str() << endl;
	printf("** Line %d, position %d: %s\n", currLine, currPos, msg);
	chex.errState=true;
	//exit( 1 );
}
void yyerror( string msg ){
   yyerror( msg.c_str() );
}

