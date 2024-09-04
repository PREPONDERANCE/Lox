package com.craftinginterpreters.lox;

public class Token {
    final TokenType tokenType;
    final String lexeme;
    final Object literal;
    final int line;

    public Token(TokenType tokenType, String lexeme, Object literal, int line) {
        this.tokenType = tokenType;
        this.lexeme = lexeme;
        this.literal = literal;
        this.line = line;
    }

    public String toString() {
        return String.format("[Token] type=%s lexeme=%s literal=%s", this.tokenType, this.lexeme, this.literal);
    }
}