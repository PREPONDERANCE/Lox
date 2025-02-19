## The Lox Language

Implement Lox language following [craftinginterpreters](https://craftinginterpreters.com/contents.html)

## Prerequisite

- Java 21
- GCC compiler
- Python 3.x


## Directory Info

```text
.
├── Challenges # Challenge problems
│
├── clox # C implementation
│ 
├── jlox/com/craftinginterpreters # Java implementation
│   ├── lox # Actual implementation
│   └── tool # GenerateAst helper
│ 
├── Lox Grammar.md # Entire lox grammar
│ 
├── Test.lox # Test script for lox language
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