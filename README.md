## The Lox Language

Implement Lox language following [craftinginterpreters](https://craftinginterpreters.com/contents.html)

## Prerequisite

- Java 21
- GCC compiler
- Python 3.x


## Directory Info

```bash
|__ clox # C implementation
|
|__ jlox/com/craftinginterpreters # Java implementation
|  |__ lox # Actual implementation
|  |__ tool # GenerateAst helper
|
|__ Lox Grammar.md # Entire lox grammar
|
|__ Test.lox # Test script for lox language
```

## Guidance

- Java Version

    ```bash
    # Enter jlox dir
    cd jlox

    # Compile jlox
    javac com/craftinginterpreters/lox/Lox.java

    # Run repl
    java com/craftinginterpreters/lox/Lox.java

    # Run file
    java com/craftinginterpreters/lox/Lox.java ../Test.lox

    # Clear compile output 
    rm -rf com/craftinginterpreters/lox/*.class 
    ```

- C Version

    > Clox supports only repl currently.

    ```bash
    # Enter clox dir
    cd clox

    # Generate makefile
    python -m gen_make

    # Compile clox
    make

    # Execute 
    ./main

    # Clear compile output
    make clean
    ```