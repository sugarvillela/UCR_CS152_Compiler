/*
    David Swanson CS152 Spring 2018
*/
#ifndef LEXPARSE_H
#define LEXPARSE_H

#include <stdio.h>
#include <cstdlib>
#include <map>
#include <string>
#include <iostream> 
#include <sstream> 
using namespace std;

 extern int currLine;
 extern int currPos;
 extern FILE * yyin;
 extern int  yylex ();
 void yyerror( string msg );
 void yyerror (const char *error);
stringstream  render;/* stream for code out */

namespace dev{//for development
   bool dispReductions, dispConstructors, dispErrors, dispSymbolTable;
   int drCount;
   void dr( string text ){//display reductions if enabled
      if( dispReductions ){
         render << dev::drCount++ << "\t" << text << "\n";
      }
   }
   void dr( string text, string* textPtr ){
      if( dispReductions ){
         render << dev::drCount++ << "\t" << text << "\t" << (*textPtr) << "\n";
      }
   }
   void dc( string text ){//display constructors if enabled
      if( dispConstructors ){
         render << "Construct: \t" << text << "\n";
      }
   }
   void dc( string text1,string text2 ){
      if( dispConstructors ){
         render << "Construct: \t"  << text1 << "\t" << text2 << "\n";
      }
   }
   void dc( string text1,string text2,string text3 ){
      if( dispConstructors ){
         render << "Construct: \t"  << text1 << "\t" << text2 << "\t" << text3 << "\n";
      }
   }
   void de( string text ){//display errors if enabled
      if( dispErrors ){
         render << "===> \t" << text << endl;
      }
   }
   void de( string text1, string text2 ){
      if( dispErrors ){
         render << "===> \t" << text1 << " \t" << text2 << endl;
      }
   }
   void de( string text1, string *text2 ){//catch null strings
      if( dispErrors ){
          if( text2 ){
              render << "===> \t"  << text1 << " \t" << (*text2) << endl;
          }
          else{
              render << "ERROR: NULL POINTER: \t"  << text1  << endl;
          }
      }
   }
   void de( string text1, string *text2, string *text3 ){//catch null strings
      if( dispErrors ){
          if( !text2 ){
              render << "ERROR: NULL FIRST POINTER: \t"  << text1  << endl;
          }
          else if( !text3 ){
              render << "ERROR: NULL SECOND POINTER: \t"  << text1  << endl;
          }
          else{
              render << "===> \t"  << text1 << " \t" << (*text2) << " \t" << (*text3) << endl;
          }
      }
   }
}

/* ========utilities========================================================= */
bool allNum(string n){//returns false on float
    for(int i=0; i< (int)n.length(); i++ ){
        if( n[i]<'0' || n[i]>'9' ){
            return false;
        }
    }
    return n.length()>0;
}
string nToStr( int n ){//some compilers don't support all std::string functions
    stringstream s; s << n; return s.str();
}
/* ========global flags for err checker====================================== */
typedef struct {
    bool inMain;    //for return
    bool inParam;   //for disallowing array in parameter
    bool inDoWhile;  //Only true between ENDLOOP and WHILE on do while
    bool funReturns;//check whether function returns
    int numMains;   //must be 1 (dup name will error first though)
    bool errState;  //yyerror sets true; multi-errors display, no code output
} checkStruct;
checkStruct chex;

/* ========symbol table parts================================================ */
enum iTypes { non, var, arr, fun };
struct identifier {
    /* Properties */
    string name;
    string alias;   //for temps: name = __temp__x, alias = sourceVarName
    int type;       //see iTypes above
    bool param;     //don't need this
    string index;   //param count, array size or array index, depending on where
    /* To do list */
    bool emitted;   //temp is used only once
    bool complement;//for unary minus
    bool negate;    //for bool not
    identifier( string setName, int setType, bool setIsParam, string setSize ) :
        name(setName), alias(""), type(setType), param(setIsParam), index(setSize), 
        emitted(false), complement(false), negate(false) 
        { dev::dc("identifier1",setName); }
        
    identifier( string setName, int setType, bool setIsParam ) :
        name(setName), alias(""), type(setType), param(setIsParam), index("0"), 
        emitted(false), complement(false), negate(false)
        { dev::dc("identifier2",setName); }
        
    identifier( string setName, int setType ) :
        name(setName), alias(""), type(setType), param(false), index("0"), 
        emitted(false), complement(false), negate(false)
        { dev::dc("identifier3",setName); }
        
    identifier( string setName, int setType, string setAlias ) :
        name(setName), alias(setAlias), type(setType), param(false), index("0"), 
        emitted(false), complement(false), negate(false)
        {
            dev::dc("identifier4",setName,setAlias); 
    }
        
    identifier( string setName, int setType, string setAlias, string setInd ) :
        name(setName), alias(setAlias), type(setType), param(false), index(setInd), 
        emitted(false), complement(false), negate(false)
        {
            dev::dc("identifier5",setName); 
        }
    void disp(){
        //if( type==non ){ return; }
        string types[]={ "non", "var","array","funct" };
        string indexSense[]={ "non", "index","array size","num params" };
        render << name << ":\ttype=" << types[type] << "\t"
            << indexSense[type] << "=" << index << "\talias=" << alias
            << "\tisParam=" << param << "\temitted=" << emitted
            << "\tcomplement=" << complement << "\tnegate=" << negate;
    }
};
class context{
   public:
    context* par;//global context
    virtual ~context(){}
    virtual void addIdent( identifier* )=0;
    virtual identifier* getIdent( string )=0;
    virtual bool iExists( string, int )=0;
    virtual bool iExists( string )=0;
    virtual string uqName()=0;
    virtual string uqName( string setAlias )=0;
    virtual string uqName( string setAlias,string setInd )=0;
    //virtual string* lastUQ()=0;
    virtual string getContextName()=0;
    virtual int getNCounter()=0;
    virtual void disp()=0;
 };
class context_obj : public context{
    string name;
    map< string, identifier* > iMap;
    int nameCounter;
    public:
        context_obj( string setContextName, context* setParent ){
            name=setContextName;    //namespace
            par=setParent;          //next on stack
            nameCounter=( par )?    par->getNCounter() :-1;//inc on uqName call
            dev::dc("context_obj",setContextName);
        }
        void addIdent( identifier* addMe ){
            if( addMe ){ 
                iMap[addMe->name]=addMe; 
            }
        }
        identifier* getIdent( string iName ){
            return iMap.count( iName )? iMap[iName] :
                ( par )? par->getIdent( iName ) : NULL;
        }
        bool iExists( string iName, int iType ){
            return ( 
                iMap.count( iName ) && iMap[iName]->type==iType 
            ) 
            || 
            ( par && par->iExists( iName, iType ) );
            
        }
        bool iExists( string iName ){
            return iMap.count( iName ) 
            || 
            ( par && par->iExists( iName ) );
        }
	    string uqName(){//increments first, then generates name
            stringstream s;
            s << "__temp__" << ++nameCounter;
            return s.str();
	    }
	    string uqName( string setAlias ){//link named var with temp var
            stringstream s;
            s << "__temp__" << ++nameCounter;
            string temp0=s.str();
            iMap[temp0]=new identifier( temp0, var, setAlias );
            return temp0;
	    }
	    string uqName( string setAlias, string setInd ){
            stringstream s;
            s << "__temp__" << ++nameCounter;
            string temp0=s.str();
            iMap[temp0]=new identifier( temp0, arr, setAlias, setInd );
            return temp0;
	    }
	    string getContextName(){
	    	return name;
	    }
	    int getNCounter(){ //returns current value without incrementing
            return nameCounter;
	    }
	    void disp(){//disp current symbol table, bottom to top
	        string parName="NULL";
	        if( par ){ 
	            parName=par->getContextName() ;
	            par->disp(); 
	        }
	        render << "Context=" << getContextName() << ", Parent=" << parName << endl;
            for( map< string, identifier* >::iterator i=iMap.begin() ; i != iMap.end() ; i++ ){
                render << "\t";
                i->second->disp();
                render << endl;
            }
	    }
};
class symbol_table : public context{
        context* sTop;
        int labelCounter;
	public:
	    symbol_table(){
	        labelCounter=-1;
	        sTop=NULL;
		    push( "global" );
		    dev::dc("symbol_table");
	    }
 	    void push( string setContextName ){
 	        sTop=new context_obj( setContextName , sTop );
	    }
	    context* pop(){		//context remains in map
	        if( !sTop ){ return NULL; }
	        context* oldTop=sTop;
	        sTop=sTop->par;
	    	return oldTop;
	    }
	    context* top(){		
	    	return sTop;
	    }
        void addIdent( identifier* addMe ){
            if( sTop ){ sTop->addIdent( addMe ); }
        }
        void setIndex( string iName, string n ){
            identifier* i;
            if( sTop && ( i=sTop->getIdent( iName ) ) ){
                i->index=n;
            }
        }
        void complement( string iName ){
            identifier* i;
            if( sTop && ( i=sTop->getIdent( iName ) ) ){
                i->complement=!i->complement;
            }
        }
        void negate( string iName ){
            identifier* i;
            if( sTop && ( i=sTop->getIdent( iName ) ) ){
                i->negate=!i->negate;
            }
        }
        identifier* getIdent( string iName ){//check if ident exists first
            return sTop? sTop->getIdent( iName ) : NULL ;
        }
        bool iExists( string iName, int iType ){
            return sTop && sTop->iExists( iName, iType );
        }
        bool iExists( string iName ){
            return sTop && sTop->iExists( iName );
        }
	    int getNCounter(){ //returns current value without incrementing
	    	return sTop? sTop->getNCounter() : 0 ;
	    }
	    string uqName(){
	        return sTop? sTop->uqName() : "NULL" ;
	    }
        string uqName( string setAlias ){
	        return sTop? sTop->uqName(setAlias) : "NULL" ;
	    }
        string uqName( string setAlias,string setInd ){
	        return sTop? sTop->uqName( setAlias, setInd) : "NULL" ;
	    }
	    string uqLab(){
	        labelCounter++;
            stringstream s;
            s << "__label__" << labelCounter;
            return s.str();
	    }
	    string getContextName(){
	    	return sTop? sTop->getContextName() : "NULL" ;
	    }
	    void info( string* d1 ){
	        if(!dev::dispErrors){return;}
	        identifier* i;
	        render << "info: ";
	        if( !d1 ){ render << "NULL; " << endl; }
	        else if( ( i=getIdent( (*d1) ) ) ){
	            render << "Logged:" << (*d1) << "; alias=" << i->alias << "; type=" << i->type << endl;
	        }
	        else {
	            render << "New:" << (*d1) << endl;
	        }	 
	    }
	    void disp(){
	        if( !dev::dispSymbolTable ){ return; }
            render << endl << "==================Display Symbol Table==================" << endl << endl;
            if(sTop){ sTop->disp(); }
            else{ dev::de("sTable.disp() no top");}//should never happen
            render << endl << "======================End Display=======================" << endl << endl;
	    }
};
/* ========global symb table stack=========================================== */
symbol_table sTable;

/* ========global error checker============================================== */
class assertive{//these return true if bad, false if good
  public:
    assertive(){ dev::dc("assertive"); }
    void arrayInParam(){
        if( chex.inParam ){
            yyerror( "Array in parameter list" );
        }
    }
    bool badDecl( string iName ){
        if( !iName.length() || sTable.iExists( iName ) ){
            yyerror( "Identifier '"+iName+"' already declared in this context" );
            return true;
        }
        return false;
    }
    bool badReference( string iName ){
        if( iName.length() && 
            ( iName[0]=='_'|| allNum(iName) || sTable.iExists( iName ) ) ){
            return false;
        }
        yyerror( "Identifier name '"+iName+"' does not exist in this context" );
        return true;
    }
    bool badReference( string iName, int iType ){//with type checking
        identifier* i;
        if( iName.length() && 
            ( iName[0]=='_'|| allNum(iName) ) ){
            return false;
        }
        if( !(i=sTable.getIdent( iName ) )){
            yyerror( "Identifier name '"+iName+"' does not exist in this context" );
            return true;
        }
        if( i->type!=iType ){
            switch ( i->type ){
                case arr:
                    yyerror("Using the array variable '"+iName+"' as scalar");
                    return true;
                case var:
                    yyerror("Using the scalar variable '"+iName+"' as array");
                    return true;
                default:
                    return true;
            }
        }
        return false;
    }
    bool nonInteger( string n ){//look for a dot
        if( !n.length() || !allNum(n) ){
            yyerror( "Positive integer required here, '"+n+"' found");
            return true;
        } 
        return false;
    }
    bool badReturn( string fName ){//main returns or non-main doesn't
        if( fName=="main" ){
            if( chex.funReturns ){
                yyerror( "Main function cannot have a return statement" );
                return true;
            }
        }
        else if( !chex.funReturns ){
            yyerror( "Function '"+fName+"' must return a value" );
            return true;
        } 
        return false;
    }
    void badMain(){
        if( !chex.numMains ){
            yyerror( "Program must contain a 'main' function" );
        }
    }
};
assertive errOn;

class code_gen{/* The code in this class gets a little complicated */
        string args[3];
        string op;
        int nArgs;
    public:
        code_gen(){ dev::dc("code_gen"); nArgs=0; }
        void negate( string iName ){//handle 'not' operator
            dev::de("gen.negate", iName );
            render << "! " << iName << ", " << iName << endl;//! dst, src
        }
        void complement( string iName ){//handle 'unary minus' operator
            string tempx=sTable.uqName();
            render << ". " << tempx << endl;
            render << "= " << tempx << ", -1" << endl;
            render << "* " << iName << ", " << iName << ", " << tempx << endl;
        }
        /* ========( n:=a ) or ( n[i]:=a ) where a is the srcName============ */
        void iAccess( string srcName ){
            identifier* src;
            if( ( src=sTable.getIdent( srcName ) ) ){//lookup text in $x
                if( !src->emitted ){//make sure the temp is declared
                    if( src->type==arr ){
                        /* Make sure index of array is declared too */
                        identifier* iIndex;
                        if( ( iIndex=sTable.getIdent( src->index ) ) && !iIndex->emitted ){
                            this->iAccess( src->index );
                        }
                        render << ". " << src->name << endl;//. __temp__0
                        render << "=[] " << src->name << ", " << src->alias << ", " << src->index << endl;//=[] __temp__28, a, __temp__27
                    }
                    else if( src->type==var ){
                        render << ". " << src->name << endl;//. __temp__0
                        if( src->alias.length() ){
                            render << "= " << src->name << ", " << src->alias << endl;//. __temp__0, a
                        }
                    }
                    src->emitted=true;
                }
                if( src->negate ){//handle 'not' operator
                    this->negate( src->name );
                    src->negate=false;
                }
                if( src->complement ){//handle 'unary minus' operator
                    this->complement( src->name );
                    src->complement=false;
                }
            }
            else{
                dev::de("gen.iAccess(srcName), didn't find srcName:", srcName);
            }
        }
        /* ========( a:=n ) or ( a[i]:=n ) where a is the dstName============ */
        void iAssign( string srcName, string dstName ){//assign to stored identifier
            identifier* dst;
            if( ( dst=sTable.getIdent( dstName ) ) ){//lookup text in $x
                if( dst->emitted ){ return; }
                if( dst->type==arr ){//[]= dst, index, srcName
                    identifier* iIndex;
                    if( ( iIndex=sTable.getIdent( dst->index ) ) && !iIndex->emitted ){
                        this->iAccess( dst->index );
                    }
                    render << "[]= " << dst->alias << ", " << dst->index << ", " << srcName << endl;
                }
                else if( dst->type==var ){//= dst, srcName
                    render << "= " << dst->alias << ", " << srcName << endl;//= a, __temp__13
                }
                else{
                    dev::de("gen.iAssign: type is not var or array: ", dst->name );
                }
                dst->emitted=true;
            } 
            else{//not necessarily an error
                dev::de("gen.iAssign(srcName, dstName), didn't find dstName:", dstName );
            }
        }
        /* ========Reunites op results with pre-existing vars================ */
        void assign(string* dstPtr, string* srcPtr ){//= __temp__0, n
            dev::de("gen:assign()", dstPtr, srcPtr );
            string dst=(*dstPtr), src=(*srcPtr);
            this->iAccess( src );
            this->iAssign( src, dst );
        }
        /* ========Ops (+*-/<>=%!||&& )================= */
        string* AL(){
            for(int i=0;i<nArgs;i++){
                dev::de("gen:AL: op="+op+", arg="+args[i] );
                this->iAccess( args[i] );
            }
            render << op << " " << args[0];
            for(int i=1;i<nArgs;i++){
                render << ", " << args[i];
            }
            render << endl;
            return new string( args[0] );
        }
        string* AL( string setOp, string* arga, string* argb ){
            dev::de("gen:AL", arga, argb );
            op=setOp;
            nArgs=3;
            args[0]=sTable.uqName("");  //setup destination
            args[1]=(*arga);
            args[2]=(*argb);
            return this->AL();
        }
        /* ========Ops (return, write, param )=============================== */
        void nop( string arga, string setOp ){
            op=setOp;
            nArgs=1;
            args[0]=arga;
            this->AL();
        }
        void nop( string* arga, string setOp ){
            this->nop( (*arga), setOp );
        }
        /* ========Ops ( function call )===================================== */
        string* fCall(string* sPtr, string* termPtr ){//returns destination temp for $$
            dev::de("gen:fCall=", sPtr, termPtr );
            string ident=(*sPtr), term=(*termPtr);
            if( errOn.badReference( ident ) ){ return sPtr; }
            this->nop( termPtr, "param" );
            string dst=sTable.uqName("");
            this->iAccess( dst );
            render << "call  " << ident << ", " << dst << endl;//call fibonacci, __temp__14
            return new string( dst );
        }
};
/* ========class of related code generating functions======================== */
code_gen gen;

class nestable{
    /* OOP requires more and more generalized function names to make an interface
    work for a variety of different operations.  
    --This may be the extreme of it:  calling functions start, middle and finish.
    To make up for this generality, each class has comments so a human reader
    can map start, middle and finish to what they actually do.
    Usually start fulfills some early task while finish finishes up
    --The idea is to push objects onto a stack as strategies for handling whatever
    part of the function we happen to be in.  
    Ex: there is a handler for each control structure type, a handler for each
    list type, and a handler that gets out of the way when things need to be
    handled differently. */
  protected:
    string str[8];
    int count;
  public:
    nestable* par;
    nestable(): count(0){}
    virtual ~nestable(){}
    virtual void start( string* sPtr )=0;
    virtual string* middle( string* nPtr )=0;
    virtual string* middle( string* sPtr, string* nPtr )=0;
    virtual string* number( string* nPtr )=0;
    virtual void finish()=0;
    virtual void finish( string* nPtr )=0;
    virtual void keyword_continue()=0;
    virtual void keyword_else()=0;

};
/* ========fork class for nested stuff======================================= */
class nop : public nestable{
    /* This class implements the functions of the abstract base class, so child
    classes only have to impliment the functions that are different. */
  public:
    nop(){ dev::dc("nop"); }
    ~nop(){}
    void start( string* sPtr ){}
    string* middle( string* nPtr ){ return nPtr; }
    string* middle( string* sPtr, string* nPtr ){ return sPtr; }
    string* number( string* nPtr ){ return nPtr; }
    void finish(){}
    void finish( string* nPtr ){}
    void keyword_continue(){
        if( par ){ par->keyword_continue(); }//forward to par until funct object errors
    }
    void keyword_else(){}//don't need error check
};
/* ========funct definition================================================== */
class funct_h : public nop {
    /* start begins the function and finish ends the function */
    string fName;
  public:
    funct_h(){ dev::dc("funct_h"); }
    ~funct_h(){}
    void start( string* sPtr ){//on funct ident
        dev::de("funct_h: start", sPtr );
        fName=(*sPtr);
        /* Status */
        chex.funReturns=false;
        if( fName=="main" ){
            chex.inMain=true;
            chex.numMains++;
        }
        /* Assert */
        if( errOn.badDecl(fName) ){ return; }
        /* Record */
        sTable.top()->addIdent( new identifier( fName, fun ) );//add this to global context
        sTable.push( fName );  //make this the top context
        /* Code gen */
        render << "func " << fName << endl;//func fibonacci
    }
    void keyword_continue(){//any non-loop forwards call up the stack to here
        yyerror( "keyword 'continue' can only be used in a loop" );
    }
    void finish(){//on end function
        dev::de("funct_h: finish" );
        /* Assert */
        errOn.badReturn( fName );
        /* Status */
        chex.inMain=false;
        /* Code gen */
        render << "endfunc" << endl;
        sTable.disp();  //display if dev::dispSymbolTable
        /* Record */
        sTable.pop();   //return to global context
    }
};
/* ========While============================================================= */
class while_h : public nop{
    /* The constructor sets up the loop.
    start writes loop-top labels; finish ends the loop */
  public:
    while_h(){
        dev::dc("while_h");
        /* get labels */
        str[0]=sTable.uqLab();
        str[1]=sTable.uqLab();
        str[2]=sTable.uqLab();
        /* Code gen */
        render << ": " << str[2] << endl;                    // : __label__2
    }
    ~while_h(){}
    void start( string* sPtr ){
        dev::de("while_h: start", sPtr );
        string tempx=(*sPtr);
        /* Code gen */
        gen.iAccess( tempx );                                   //make sure the temp var is emitted
        render << "?:= " << str[0] << ", " << tempx << endl; // ?:= __temp__5, __label__0
        render << ":= " << str[1] << endl;                   //:= __label__1
        render << ": " << str[0] << endl;                    //: __label__0
    }
    void keyword_continue(){//TODO: add continue reduction
        render << ":= " << str[2] << endl;// : __label__2
    }
    void finish(){
        dev::de("while_h: finish" );
        /* Code gen */
        render << ":= " << str[2] << endl;                   //:= __label__2
        render << ": " << str[1] << endl;                    //: __label__1
    }
};
/* ========Do While========================================================== */
class doWhile_h : public nop{
    /* The constructor sets up the loop, writes loop-top label.
    start is dummy, finish writes loop-bottom labels; finish(result) runs conditional */
  public:
    doWhile_h(){
        dev::dc("doWhile_h");
        /* get labels */
        str[0]=sTable.uqLab();//on continue
        str[1]=sTable.uqLab();//loop top
        /* Code gen */
        render << ": " << str[1] << endl;                    // : __label__1
    }
    ~doWhile_h(){}
    void start( string* sPtr ){                               //BEGINLOOP
        dev::de("doWhile_h: start (dummy)", sPtr );
    }
    void keyword_continue(){//TODO: add continue reduction
        render << ":= " << str[0] << endl;// : __label__2
    }
    void finish(){                                                 //ENDLOOP
        dev::de("doWhile_h: finish" );
        chex.inDoWhile=true;                                    //proper action for 'while'
        /* Code gen */
        render << ": " << str[0] << endl;                    //: __label__0
    }
    void finish( string *sPtr ){                                        //while 
        dev::de("doWhile_h: finish", sPtr );
        string tempx=(*sPtr);
        /* Code gen */
        gen.iAccess( tempx );                                   //make sure the temp var is emitted
        render << "?:= " << str[1] << ", " << tempx << endl; // ?:= __temp__5, __label__1
        chex.inDoWhile=false;                                   //proper action for 'while'
    }
};
/* ========If-Else=========================================================== */
class if_h : public nop{
    /* The constructor sets up the conditional.
    start writes top labels; finish ends the conditional.
    The labels work out right, whether you call else or not */
  public:    
    if_h(){//if
        dev::dc("if_h");
        /* get labels */
        str[0]=sTable.uqLab();
        str[1]=sTable.uqLab();
        str[2]=sTable.uqLab();
        count=0;
    }
    ~if_h(){}
    void start( string* sPtr  ){//then
        dev::de("if_h: start", sPtr );
        string tempx=(*sPtr);
        /* Code gen */
        gen.iAccess( tempx );                                        //make sure the temp var is emitted
        render << "?:= " << str[count++] << ", " << tempx << endl;   //if true go to IF          0
        render << ":= " << str[count] << endl;                       //go to ELSE or ENDIF       1
        render << ": " << str[count-1] << endl;                      //label IF, do if           0
    }
    void keyword_else(){
        /* Code gen */
        render << ":= " << str[++count] << endl;                     //finished if, go to ENDIF  2
        render << ": " << str[count-1] << endl;                      //label ELSE, do else       1
    }
    void finish(){//endif
        dev::de("if_h: finish" );
        /* Code gen */
        render << ": " << str[count] << endl;                        //label ENDIF               1 if no 'else', else 2
    }
};
/* ========ident lists======================================================= */
class list_h : public nop{//param_h, local_h use same start 
  public:
    list_h(){ dev::dc("local_h"); }
    ~list_h(){}
    void start( string* sPtr ){
        dev::de("list_h: start", sPtr );
        str[count++]=(*sPtr);
    }
};
class param_h : public list_h{
    /* start adds vars to the list and finish logs the count on end_param */
  public:
    param_h(){ dev::dc("param_h"); }
    ~param_h(){}
    void finish(){
        dev::de("param_h: finish" );
        string ident;
        for(int i=0;i<count;i++){
            ident=str[i];
            /* Assert */
            if( errOn.badDecl(ident) ){ count=0; return; }//bad if shadows a function name
            /* Record */
            sTable.top()->addIdent( new identifier( ident, var, true ) );//isParam=true
            /* Code gen */
            render << ". " << ident << endl << "= " << ident << ", $" << i << endl;// . k  /n      = k, $0
        }
        sTable.setIndex( sTable.getContextName(), nToStr( count ) );//set number of parameters
        count=0;
    }
};
class local_h : public list_h{
    /* start adds vars to a list and finish logs type at each decl reduction */
  public:
    local_h(){ dev::dc("local_h"); }
    ~local_h(){}
    /* ========Scalar decl======== */
    void finish(){
        dev::de("local_h: finish" );
        string ident;
        for(int i=0;i<count;i++){
            ident=str[i];
            /* Assert */
            if( errOn.badDecl(ident) ){ count=0; return; }
            /* Record */
            sTable.top()->addIdent( new identifier( ident, var ) );
            /* Code gen */
            render << ". " << ident << endl; // . k 
        }
        count=0;
    }
    /* ========Array decl======== */
    void finish( string* nPtr ){
        dev::de("local_h: finish: array handler", nPtr );
        string n=(*nPtr);
        string ident;
        for(int i=0;i<count;i++){
            ident=str[i];
            /* Assert */
            if( errOn.badDecl(ident) || errOn.nonInteger(n) ){ count=0; return; }
            /* Record */
            sTable.top()->addIdent( new identifier( ident, arr, false, n ) );
            /* Code gen */
            render << ".[] " << ident << ", " << n << endl;// .[] a, 1000
        }
        count=0;
    }
};
/* ========function body===================================================== */
class fBody_h: public nop{
    /* This is the class that gets out of the way.
    It is instantiated at the base of the val stack, with other handlers
    pushed on top of it.
    start is called at every ident reduction.  Since we don't know what kind
    of variable to expect (var or array) we need to wait for var to reduce
    So start does nothing and middle handles the variable */
  private:
  public:
    fBody_h(){ dev::dc("fBody_h"); }
    ~fBody_h(){}
    void start( string* sPtr ){
        dev::de("fBody_h: start (dummy)", sPtr );
    }
    string* middle( string* sPtr ){//this handles variables
        dev::de("fBody_h: middle: var handler", sPtr );
        /* Assert */
        if( errOn.badReference( (*sPtr), var ) ){ return sPtr; }
        /* Record:
        sTable.uqName creates a new identifier that maps the temp var to its
        named var, then returns the temp var for $$. */
        return new string( sTable.uqName( (*sPtr) ) );
    }
    string* middle( string* sPtr, string* nPtr ){
        dev::de("fBody_h: middle:array handler", sPtr, nPtr );
        /* Assert */
        if( errOn.badReference( (*sPtr), arr ) ){ return sPtr; }
        /* Record:
        sTable.uqName creates a new identifier that maps the temp var to its
        named var (saving the index too), then returns the temp var for $$. */
        return new string( sTable.uqName( (*sPtr), (*nPtr) ) );
    }
    string* number( string* nPtr ){
        return this->middle( nPtr );
    }
};
/* ========Read multiple inputs to list of var and/or array================== */
class read_h: public fBody_h{
  public:
    read_h(){ dev::dc("read_h"); }
    ~read_h(){}
    string* middle( string* sPtr ){//this handles variables
        dev::de("read_h: middle: var handler", sPtr );
        /* Assert */
        if( errOn.badReference( (*sPtr), var ) ){ return sPtr; }
        /* Record */
        string tempx=sTable.uqName( (*sPtr) );
        str[count++]=tempx;
        return new string( tempx );
    }
    string* middle( string* sPtr, string* nPtr ){
        dev::de("read_h: middle:array handler", sPtr, nPtr );
        /* Assert */
        if( errOn.badReference( (*sPtr), arr ) ){ return sPtr; }
        /* Record */
        string tempx=sTable.uqName( (*sPtr), (*nPtr) );
        str[count++]=tempx;
        return new string( tempx );
    }
    string* number( string* nPtr ){
        dev::de("read_h: number handler", nPtr );
        return new string( sTable.uqName( (*nPtr) ) );
    }
    void finish(){
        dev::de( "read_h: finish" );
        string dst;
        for(int i=0;i<count;i++){
            /* Code gen */
            dst=str[i];
            identifier *idst;
            if( ( idst=sTable.getIdent( dst ) ) ){
                string uq=sTable.uqName("");
                gen.iAccess( uq );
                render << ".< " << uq << endl;//.< dst
                gen.iAssign( uq, dst );
            }
        }
        count=0;
    }
};
/* ========write multiple var and/or array to output========================= */
class write_h: public read_h{
  public:
    write_h(){ dev::dc("write_h"); }
    ~write_h(){}
    void finish(){
        dev::de( "read_h: finish" );
        for(int i=0;i<count;i++){
            gen.nop( str[i], ".>" );
        }
        count=0;
    }
};
/* ========class for flow and val stacks===================================== */
class nestHandler{
  public:
    nestable* hTop;
    nestHandler(){
        dev::dc("nestHandler");
        hTop=new nop();
        hTop->par=NULL;
    }
    nestHandler( nestable* setTop ){
        hTop=setTop;
        hTop->par=NULL;
    }
    ~nestHandler(){
        while(pop());
    }
    nestable* chooseHandler( string hName ){
        if( hName=="funct" ){return new funct_h();}             //On funct def 
        if( hName=="begin_param" ){return new param_h();}
        if( hName=="begin_local" ){ return new local_h();}
        if( hName=="if" ){return new if_h();}
        if( hName=="while" ){return new while_h();}
        if( hName=="do_while" ){return new doWhile_h();}
        if( hName=="read" ){return new read_h();}
        if( hName=="write" ){return new write_h();} 
        dev::de("chooseHandler: bad name: ", hName);
        return new nop();
    }
    void push( string hName ){
         nestable* newTop=chooseHandler( hName );
         newTop->par=hTop;
         hTop=newTop;
    }
    bool pop(){
        if( hTop ){
            nestable* oldTop=hTop;
	        hTop=hTop->par;
	    	delete oldTop; 
	    	return true;
        }
        return false;
    }
    void start( string* sPtr ){
        //dev::de("flow|val stack: start", sPtr );
        if( hTop ){ hTop->start( sPtr );}
    }
    string* middle( string* nPtr ){
        return ( hTop )? hTop->middle( nPtr ) : new string();
    }
    string* middle( string* sPtr,string* nPtr ){
        return ( hTop )? hTop->middle( sPtr, nPtr ) : new string();
    }
    string* number( string* nPtr ){
        return ( hTop )? hTop->number( nPtr ) : new string();
    }
    void finish(){
        //dev::de("flow|val stack: finish" );
        if( hTop ){ hTop->finish();}
    }
    void finish( string* nPtr ){
        //dev::de("nestHandler: finish", nPtr );
        if( hTop ){ hTop->finish( nPtr );}
    }
    void keyword_continue(){
        //dev::de("flow|val stack: continue" );
        if( hTop ){ hTop->keyword_continue();}
    }
    void keyword_else(){
        //dev::de("flow|val stack: else" );
        if( hTop ){ hTop->keyword_else();}
    }
};
/* ========global flow control stack========================================= */
nestHandler flow;  //flow of control: loops and ifelse
/* ========global ident list stack=========================================== */
/* fBody_h on stack; push special handlers on top of it */
nestHandler vals( new fBody_h() ); //param and local lists, or var reference

#endif
