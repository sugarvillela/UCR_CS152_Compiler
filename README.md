# UCR_CS152_Compiler
MiniL language to mil language using Flex and Bison

    David Swanson CS152 Spring 2018
    
    This program reads code in an input language, MiniL and translates it to 
    a lower-level language, mil.  A mil interpreter runs the generated mil code.
    
    Compiling/Translating is just string manipulation: mapping an input string 
    to an output string.  The reason it is complicated is that to do a good 
    translation we need to know where the 'ideas' begin and end.  For that we 
    use a so-called context free grammar to articulate the recursive structure 
    of the code.  Bison is the easiest way to do this.
    
    Most programs of this type would push object pointers onto Bison's $ stack.
    I personally had trouble with Bison because the compiler kept detecting 
    duplicate definitions of the stuff in the header files.  Rather than sort
    that out, I did the whole thing with string pointers.  There is no parse tree,
    except the one that exists implicitly, and is parsed once by Bison.  Thus 
    the program does everything in one pass.  And since I did not use Bison's 
    stack, I built my own data structures to hold nested control structures in
    the language.  
        
    How the program works:  
    As stated, there is no explicit parse tree.  Instead, a string pointer 
    to a temp variable name, like __temp__0, is pushed to $$ at certain reductions.
    The symbol table holds info about named vars, temps, numbers and functions.
    To use the temp variable in $$, the program looks up the traits of the temp
    var in the symbol table and decides then what to do with it.
    
    This solves the problem of vars vs arrays, and var access vs assign.  The 
    program generates the temp name when bison finds a var, but waits to emit it
    until it knows whether to access the var or assign something to it.  At that
    time it emits code for either a var or array, depending on the type stored
    in the symbol table.
    
    The program uses a linked stack for the symbol table. That's overkill for a 
    longuage with only two context levels: global and function.  But I thought to 
    reuse the code later for a bigger language.  Allowing for classes, functions 
    and blocks or lambda functions, there is potential for many context levels.
    Using the stack structure, a context can 'see' its parent contexts, but does 
    not know about its siblings. This works for checking identifier names etc.
    
    The symbol table stack holds objects of type 'context', each of which holds
    a map of type 'struct identifier'.  Each identifier struct holds the name 
    and type of an identifier.  It also holds other data; see code for details.
    
    There are two other linked stacks, both of type 'nest_handler'.  Eachs holds
    objects of type 'nestable', which are handlers for the various control
    structures in the language.  One stack is called 'flow', and handles 
    function, while, if-else etc.  The other is called 'vals', and handles 
    ident' and 'var' reductions.
    
    This is a strategy design pattern: whatever object is at the top of the 
    stack is in charge.  There are others waiting to take over once the top is 
    popped.  It is how the program manages an indeterminate number of nested
    loops, and if-elses, keeping track of the label names for each.
    
    One other class 'code_gen' handles operations like add, mult, return etc.
    
    This program has been tested with the given test files, plus I tested it 
    with nested loops and conditionals, do-while within while, if-else within
    if etc.  It also supports function calls within function calls:
    x:=first( second( n ) );  And it supports multi-var read and write
    
    I added handlers for NOT and unary minus.  They work by negating or 
    complementing a temp var after is is created and before it is used.
    But I don't think mil_run interpreter supports negative numbers...
