#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#define UINT8_COUNT (UINT8_MAX + 1)

typedef struct
{
    Token previous;
    Token current;
    bool hadError;  // Indicating a compile error
    bool panicMode; // Used for error recovery
} Parser;

typedef struct
{
    Token name; // Name of the local variable
    int depth;  // Depth inside the stack (the number of blocks preceding it)
} Local;

typedef struct
{
    Local locals[UINT8_COUNT]; // Simulate stack
    int localCount;            // Total number of locals
    int scopeDepth;            // Total depths of locals
} Compiler;

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

// A function pointer with no arguments and output void
typedef void (*ParseFn)(bool canAssign);

typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence; // Infix precedence
} ParseRule;

Parser parser;
Compiler *current = NULL;
Chunk *compilingChunk;

static void initCompiler(Compiler *compiler)
{
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static Chunk *currentChunk()
{
    return compilingChunk;
}

static void errorAt(Token *token, const char *message)
{
    // When the parser is in panic mode, we ignore the errors.
    // Panic is recovered in statement judgement.
    if (parser.panicMode)
        return;
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else if (token->type != TOKEN_ERROR)
        fprintf(stderr, " at '%.*s'", token->length, token->start);

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char *message)
{
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char *message)
{
    errorAt(&parser.current, message);
}

static void advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scanToken();

        if (parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(parser.current.start);
    }
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

static void consume(TokenType type, const char *message)
{
    if (check(type))
    {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX)
    {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitLoop(uint8_t loopStart)
{
    emitByte(OP_LOOP);

    // Add the 16-bit offset
    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX)
        error("Loop body too large");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static void emitReturn()
{
    emitByte(OP_RETURN);
}

static void endCompiler()
{
    emitReturn();
}

static void expression();
static void statement();
static void declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

static void beginScope()
{
    current->scopeDepth++;
}

static void endScope()
{
    current->scopeDepth--;

    // After we end a scope, we need to clear this scope's
    // local variables. Also, since the values of these local
    // vars sit on the vm's stack, we need to pop them out.
    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth)
    {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
        declaration();

    consume(TOKEN_RIGHT_BRACE, "Missing '}' after block statement.");
}

static uint8_t identifierConstant(Token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifierEqual(Token *a, Token *b)
{
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void addLocal(Token name)
{
    // Add a local var to the locals field in the compiler
    Local *local = &current->locals[current->localCount++];

    // Check whether the number of local vars exceeds the limit
    if (current->localCount >= UINT8_COUNT)
    {
        error("Too many local variables in function.");
        return;
    }

    local->name = name;
    local->depth = -1;
    // Mark uninitialized
}

static void declareVariable()
{
    // Declaring a variable is when it's added to a scope

    if (current->scopeDepth == 0)
        return;

    Token *name = &parser.previous;
    // Check whether a local with the same name as
    // `name->start` exists

    for (int i = current->localCount - 1; i >= 0; i--)
    {
        Local *local = current->locals + i;

        // This seems redundant but if our parser enters a panic mode
        // because of using uninitialized var (`var a = a` in a block),
        // we need to skip these values on the next traverse.
        // Consider inside a block, `{ var a = a; {var b = a; var c = c;} }`
        // `b` needs to skip `a` in the first block if there's an `a` on the
        // global scope.
        if (local->depth != -1 && local->depth < current->scopeDepth)
            break;

        if (identifierEqual(name, &local->name))
            error("Already a variable with this name exists in the scope");
    }

    addLocal(*name);
}

static uint8_t parseVariable(const char *message)
{
    // The idea is that we store the variable name as
    // a string constant so that we can look it up with
    // our hash table.
    consume(TOKEN_IDENTIFIER, message);

    // Local variable, return fake index 0
    declareVariable();
    if (current->scopeDepth > 0)
        return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized()
{
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
    // Defining a variable is when it's available to use

    // If we're seeing a local variable, skip the definition
    if (current->scopeDepth > 0)
    {
        markInitialized();
        return;
    }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

// Search in `compiler`'s local var field
static int resolveLocal(Compiler *compiler, Token *name)
{
    for (int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local *local = compiler->locals + i;
        if (identifierEqual(name, &local->name))
        {
            if (local->depth == -1)
                error("Can't read local variable in its own initializer.");
            return i;
        }
    }
    return -1;
}

static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;

    int arg = resolveLocal(current, &name);
    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
        // For local vars, the arg is the index at
    }
    else
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    // Add one look ahead for possible assignment
    if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else
        emitBytes(getOp, (uint8_t)arg);
}

static void variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

static void varDeclaration()
{
    // A lot like reading constants except for different op code.
    uint8_t global = parseVariable("Expect identifier after 'var'.");

    if (match(TOKEN_EQUAL))
        expression();
    else
        emitByte(OP_NIL);
    consume(TOKEN_SEMICOLON, "Expect ';' after var declaration");

    defineVariable(global);
}

static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Missing ';' after expression.");
    emitByte(OP_POP);
    // For now without function support,
    // we just pop the values out.
}

static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Missing ';' after print.");
    emitByte(OP_PRINT);
}

static void patchJump(int offset)
{
    // Minus 2: skipping the 16-bit offset.
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX)
        error("Too much code to jump");

    currentChunk()->code[offset] = jump >> 8 & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

static void ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Missing '(' after if.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Missing ')' after if condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    statement();
    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if (match(TOKEN_ELSE))
        statement();
    patchJump(elseJump);
}

static void whileStatement()
{
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Missing '(' after while.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Missing ')' after while condition");

    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);

    patchJump(endJump);
    emitByte(OP_POP);
}

static void forStatement()
{
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Missing '(' after for.");

    // Parsing initializers, execute only once.
    if (match(TOKEN_SEMICOLON))
        ; // No initializer
    else if (match(TOKEN_VAR))
        varDeclaration();
    else
        expressionStatement();

    int loopStart = currentChunk()->count;

    // Parsing conditions, execute multiple times
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON))
    {
        // For loop condition
        // We didn't use `expressionStatement` here
        // since we need the expression result to determine
        // whether to jump. An `expressionStatement`
        // will emit an OP_POP, jeopardizing the work flow.
        expression();
        consume(TOKEN_SEMICOLON, "Missing expression in for loop condition");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    // Parseing incremental expressions
    if (!match(TOKEN_RIGHT_PAREN))
    {
        int bodyJump = emitJump(OP_JUMP);
        int incrementLoop = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Missing ')' after for loop.");

        emitLoop(loopStart);
        loopStart = incrementLoop;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if (exitJump != -1)
    {
        patchJump(exitJump);
        emitByte(OP_POP);
    }
    endScope();
}

static void synchronize()
{
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF)
    {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;

        switch (parser.current.type)
        {
        case TOKEN_RETURN:
        case TOKEN_CLASS:
        case TOKEN_PRINT:
        case TOKEN_WHILE:
        case TOKEN_FUN:
        case TOKEN_FOR:
        case TOKEN_VAR:
        case TOKEN_IF:
            return;
        default:;
        }

        advance();
    }
}

static void declaration()
{
    if (match(TOKEN_VAR))
        varDeclaration();
    else
        statement();

    if (parser.hadError)
        synchronize();
}

static void statement()
{
    if (match(TOKEN_PRINT))
        printStatement();
    else if (match(TOKEN_LEFT_BRACE))
    {
        beginScope();
        block();
        endScope();
    }
    else if (match(TOKEN_IF))
        ifStatement();
    else if (match(TOKEN_WHILE))
        whileStatement();
    else if (match(TOKEN_FOR))
        forStatement();
    else
        expressionStatement();
}

// Infix expression
static void and_(bool canAssign)
{
    // As we parse `and` keyword, the left operand
    // has already being parsed and evaluated (during
    // execution). Same for `or`.

    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

// Infix expression
static void or_(bool canAssign)
{
    // We could implement an OP_JUMP_IF_TRUE op code
    // which reduces the jump overheads
    // But for demonstration purpose, jump false & jump
    // could implement a jump true.

    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

// Infix expression
// This function takes place after prefix expression.
static void binary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)rule->precedence + 1);
    // The reason why we add 1 is that binary operations
    // are left-associative meaning 1 + 2 + 3 is actually
    // ((1 + 2) + 3). Thus, we only want 2 instead of the
    // rest on parsing the initial 1.

    switch (operatorType)
    {
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_BANG_EQUAL:
        emitBytes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_GREATER_EQUAL:
        emitBytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS_EQUAL:
        emitBytes(OP_GREATER, OP_NOT);
        break;
    default:
        return;
    }
}

// Prefix expression
static void literal(bool canAssign)
{
    switch (parser.previous.type)
    {
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    default:
        return;
    }
}

// Prefix expression
static void number(bool canAssign)
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

// Prefix expression
static void string(bool canAssign)
{
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

// Prefix expression
static void grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// Prefix expression
static void unary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType)
    {
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    default:
        return;
    }
}

// Map each token to a specific rule for prefix & infix parsing.
// These sets of rules are made specifically for expression.
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

// Parse precedence >= `precedence`
static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefixFn = getRule(parser.previous.type)->prefix;
    if (prefixFn == NULL)
    {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixFn(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();
        ParseFn infixFn = getRule(parser.previous.type)->infix;
        infixFn(canAssign);
    }

    // The idea here is if we cannot even parse '=' in the
    // while loop, then no where in the code are we able to
    // parse it. Thus, it's definitely an invalid assignment.
    if (canAssign && match(TOKEN_EQUAL))
        error("Invalid assignment target.");
}

static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

bool compile(const char *source, Chunk *chunk)
{
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TOKEN_EOF))
        declaration();

    endCompiler();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
        disassembleChunk(currentChunk(), "code");

#endif
    return !parser.hadError;
}