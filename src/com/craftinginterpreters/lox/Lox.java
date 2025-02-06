package com.craftinginterpreters.lox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Lox {
    private static final Interpreter interpreter = new Interpreter();

    static boolean hadError = false;
    static boolean hadRuntimeError = false;

    static void error(int line, String message) {
        report(line, "", message);
    }

    static void error(Token token, String message) {
        if (token.tokenType == TokenType.EOF) {
            report(token.line, "at end", message);
        } else {
            report(token.line, "at '" + token.lexeme + "'", message);
        }
    }

    static void runtimeError(RuntimeError error) {
        System.err.println(
                String.format("%s \nRuntimeError: [line %d] invalid symbol \"%s\"", error.getMessage(),
                        error.token.line,
                        error.token.lexeme));
        hadRuntimeError = true;
    }

    private static void report(int line, String where, String message) {
        System.out.println(String.format("[line %d] Error %s: %s", line, where, message));
        hadError = true;
    }

    private static void run(String source) throws IOException {
        Scanner scanner = new Scanner(source);
        List<Token> tokens = scanner.scanTokens();

        Parser parser = new Parser(tokens);
        List<Stmt> statements = parser.parse();
        Resolver resolver = new Resolver(interpreter);
        resolver.resolve(statements);

        // Stop if there was a syntax error.
        if (hadError)
            return;

        interpreter.interpret(statements);
    }

    private static void runPrompt() throws IOException {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));

        for (;;) {
            System.out.printf("> ");
            String line = reader.readLine();
            if (line == null)
                break;
            run(line);
            hadError = false;
        }
    }

    private static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));

        if (hadError)
            System.exit(65);

        if (hadRuntimeError)
            System.exit(70);
    }

    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
            System.out.println("Usage: jloc [script]");
            System.exit(64);
        } else if (args.length == 1) {
            runFile(args[0]);
        } else {
            runPrompt();
        }
    }

}