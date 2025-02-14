#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct
{
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static char advance()
{
    /*
    Another way to write it:

    scanner.current++;
    return scanner.current[-1];
    */
    return *scanner.current--;
}

static char peek()
{
    return *scanner.current;
}

static char peekNext()
{
    if (isAtEnd())
        return '\0';
    return *(scanner.current + 1);
}

static bool match(char expect)
{
    if (isAtEnd())
        return false;
    if (*scanner.current == expect)
    {
        advance();
        return true;
    }
    return false;
}

static Token makeToken(TokenType type)
{
    Token token;

    token.type = type;
    token.line = scanner.line;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);

    return token;
}

static Token errorToken(const char *message)
{
    Token token;

    token.line = scanner.line;
    token.start = message;
    token.length = (int)strlen(message);
    token.type = TOKEN_ERROR;

    return token;
}

// Handle all useless characters: whitespace,
// line break, \t, \r, comments.
static void skipWhitespace()
{
    char c;

    for (;;)
        switch (c = peek())
        {
        case ' ':
        case '\t':
        case '\r':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        case '/':
            if (peekNext() == '/')
            {
                while (!isAtEnd() && c != '\n')
                    advance();

                if (!isAtEnd())
                {
                    scanner.line++;
                    advance();
                }
            }
            else
                return;
            break;
        default:
            return;
        }
}

static Token string()
{
    char c;
    while (!isAtEnd() && (c = peek()) != '"')
    {
        if (c == '\n')
            scanner.line++;
        advance();
    }

    if (peek() != '"')
        return errorToken("Unterminated string.");

    // Different from jlox, we need both enclosing quotes
    advance();
    return makeToken(TOKEN_STRING);
}

static Token number()
{
    while (isDigit(peek()))
        advance();

    if (peek() == '.' && isDigit(peekNext()))
    {
        do
            advance();
        while (isDigit(peek()));
    }

    return makeToken(TOKEN_NUMBER);
}

static TokenType checkKeyword(int start, int length, const char *rest, TokenType target)
{
    if (scanner.current - scanner.start == (start + length) && memcmp(scanner.start + start, rest, length) == 0)
        return target;

    return TOKEN_IDENTIFIER;
}

// Simulate that of trie tree since we don't
// have a built-in hash table in C.
static TokenType identifierType()
{
    switch (*scanner.start)
    {
    case 'a':
        return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c':
        return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
        return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'i':
        return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n':
        return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o':
        return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p':
        return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
        return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 'v':
        return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
        return checkKeyword(1, 4, "hile", TOKEN_WHILE);

    case 'f':
        if (scanner.current - scanner.start > 1)
        {
            switch (*(scanner.start + 1))
            {
            case 'o':
                return checkKeyword(2, 1, "r", TOKEN_FOR);
            case 'a':
                return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'u':
                return checkKeyword(2, 1, "n", TOKEN_FUN);
            }
        }
        break;
    case 't':
        if (scanner.current - scanner.start > 1)
        {
            switch (*(scanner.start + 1))
            {
            case 'r':
                return checkKeyword(2, 2, "ue", TOKEN_TRUE);
            case 'h':
                return checkKeyword(2, 2, "is", TOKEN_THIS);
            }
        }
        break;
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    while (isAlpha(peek()) || isDigit(peek()))
        advance();

    return makeToken(identifierType());
}

Token scanToken()
{
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd())
        return makeToken(TOKEN_EOF);

    char c = advance();

    if (isDigit(c))
        return number();
    if (isAlpha(c))
        return identifier();

    switch (c)
    {
    // Single token
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE);
    case ';':
        return makeToken(TOKEN_SEMICOLON);
    case ',':
        return makeToken(TOKEN_COMMA);
    case '.':
        return makeToken(TOKEN_DOT);
    case '+':
        return makeToken(TOKEN_PLUS);
    case '-':
        return makeToken(TOKEN_MINUS);
    case '*':
        return makeToken(TOKEN_STAR);
    case '/':
        return makeToken(TOKEN_SLASH);

    // Double tokens
    case '=':
        return match('=') ? makeToken(TOKEN_EQUAL_EQUAL) : makeToken(TOKEN_EQUAL);
    case '!':
        return match('=') ? makeToken(TOKEN_BANG_EQUAL) : makeToken(TOKEN_BANG);
    case '>':
        return match('=') ? makeToken(TOKEN_GREATER_EQUAL) : makeToken(TOKEN_GREATER);
    case '<':
        return match('=') ? makeToken(TOKEN_LESS_EQUAL) : makeToken(TOKEN_LESS);

    // Literal values
    case '"':
        return string();
    }

    return errorToken("Unexpected character.");
}