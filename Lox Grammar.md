### Expr grammar

```text
expression  -> assignment;
assignment  -> (call ".")? IDENTIFIER "=" assignment | logical_or;
logical_or  -> logical_and ("or" logical_and)*;
logical_and -> equality ("and" equality)*;
equality    -> comparison (("==" | "!=") comparison)*;
comparison  -> term ((">" | "<" | "<=" | ">=") term)*;
term        -> factor (("+" | "-")factor)*;
factor      -> unary (("*" | "/") unary)*;
unary       -> ("-" | "!") unary | call;
call        -> primary ("(" arguments? ")" | "." IDENTIFIER)*;
arguments   -> expression ("," expression)*;
primary     -> NUMBER | STRING | "nil" | "true" | "false" | IDENTIFIER
               | "(" expression ")" | "super" "." IDENTIFIER;
```

The assignment and unary rules are written differently than others because of their right associativity.

The former two evaluate from right to left while the rest evaluate from left to right.

The reference to `call` allows any high-precedence expression before the last dot, including any number of *getters*

### Statement grammar

```text
program     -> declaration* EOF;
declaration -> classDecl | funDecl | varDecl | statement;
classDecl   -> "class" IDENTIFIER ("<" IDENTIFIER)? "{" function* "}";
funDecl     -> "fun" function;
function    -> IDENTIFIER "(" parameters? ")" block;
parameters  -> IDENTIFIER ("," IDENTIFIER)*;
varDecl     -> "var" IDENTIFIER ("=" expression)? ";";
statement   -> printStmt | exprStmt | ifStmt | whileStmt 
 							 | forStmt | returnStmt | block;
exprStmt    -> expression ";";
printStmt   -> "print" expression ";";
ifStmt      -> "if" "(" expression ")" statement ("else" statement)?;
whileStmt   -> "while" "(" expression ")" statment;
forStmt     -> "for" "(" (varDecl | exprStmt | ";") 
               expression? ";" 
							 expression? ")" statement;
returnStmt  -> "return" expression? ";";
block       -> "{" declaration* "}";
```

