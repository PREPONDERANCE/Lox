## Answers

1. One plausible way:

    ```text
    expr → expr ( "(" ( expr ( "," expr )* )? ")" | "." IDENTIFIER )+
     | IDENTIFIER
     | NUMBER

    expr -> IDENTIFIER
    expr -> NUMBER
    expr -> calls

    calls -> calls call
    calls -> call

    call -> "." IDENTIFIER
    call -> "(" ")"
    call -> "(" argument ")"

    argument -> expr
    argument -> argument "," expr
    ```

    The problem has been how to use recursion to represent the notation of **"more"**.

2. Unanswered due to inadequate experience in functional programming language

3. Code as below

    ```java
    package com.craftinginterpreters.lox;

    public class RpnPrinter implements Expr.Visitor<String> {
        String print(Expr expr) {
            return expr.accept(this);
        }

        @Override
        public String visitBinaryExpr(Expr.Binary expr) {
            return expr.left.accept(this) + " " + expr.right.accept(this) + " " + expr.operator.lexeme;
        }

        @Override
        public String visitUnaryExpr(Expr.Unary expr) {
            return expr.operator.lexeme + expr.right.accept(this);
        }

        /* The answer writes differently
         * @Override
         * public String visitUnaryExpr(Expr.Unary expr) {
         *     String operator = expr.operator.lexeme;
         *     if (expr.operator.type == TokenType.MINUS) {
         *         operator = "~";
         *     }
         *     return expr.right.accept(this) + " " + operator;
         * }
         * 
         */


        @Override
        public String visitLiteralExpr(Expr.Literal expr) {
            return expr.value.toString();
        }

        @Override
        public String visitGroupingExpr(Expr.Grouping expr) {
            return expr.expression.accept(this);
        }

        public static void main(String[] args) {
            Expr expr = new Expr.Binary(
                    new Expr.Grouping(
                            new Expr.Binary(
                                    new Expr.Literal(1),
                                    new Token(TokenType.PLUS, "+", null, 1),
                                    new Expr.Literal(2))),
                    new Token(TokenType.STAR, "*", null, 1),
                    new Expr.Grouping(
                            new Expr.Binary(
                                    new Expr.Literal(4),
                                    new Token(TokenType.MINUS, "-", null, 1),
                                    new Expr.Literal(3))));

            Expr expression = new Expr.Binary(
                    new Expr.Unary(
                            new Token(TokenType.MINUS, "-", null, 1),
                            new Expr.Literal(123)),
                    new Token(TokenType.STAR, "*", null, 1),
                    new Expr.Grouping(
                            new Expr.Literal("str")));

            RpnPrinter rpnPrinter = new RpnPrinter();

            System.out.println(rpnPrinter.print(expr));
            System.out.println(rpnPrinter.print(expression));
        }
    }

    ```
