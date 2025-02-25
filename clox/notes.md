1. Allow instructions to have **operands**. For example, the `OP_CONSTANT` instruction is followed by a value indicating the index of the last inserted value. At evaluation time, the index will be read and processed.

2. A `do..while...` loop in the macro allows for a block of statements without the semicolons.

3. Prefix expression means we can judge what kind of expression this expression belongs to by just looking at the first token, while an infix expression means we can only determine the expression kind after consuming the first token.

4. `union` is a lot like `struct` except that the fields in `union` overlap with each other, meaning the memory usage in `union` equals the largest field.

5. `static void runtimeError(const char* format, ...)` means this function receives a varying number of arguments. `va_list args` initializes the arguments; `vfprintf(stderr, format, args)` works just like `fprintf` in that it takes in the file pointer (in this case, the stderr) and format string (for example, "%d %d %s") and the arguments. `va_end` is sort of like freeing the arguments.

6. The instructions our vm contains might not be aligned with the actual instructions. `a <= b` is equivalent to `!(a > b)`; `a != b` equivalent to that of `!(a == b)`.

7. When comparing the values, we cannot use `memcmp` to compare two value structs since the padding bits may differ.

8. Casting a pointer is really about changing the way the pointer looks at the memory. For example, a variable of type `char` named `c` is pointed by `char *` pointer `d`, namely `char* d = &c;`. `d` looks at only one byte of memory. But when we do `* (int *) d`, we are looking at 4 bytes of memory since `int` occupies 4 bytes.

9. The `tombstone` mechanism: suppose a hash table as `['a', 'b', 'c']`. All the elements' hash values are the same. If we delete entry 'b' entirely (key set to NULL, value set to NIL), we end up unable to find entry 'c' since the second slot is treated empty. As a result, on deleting entry 'b', we set the key to NULL but value to BOOL(true) (anything other than NIL) to mark this position as tombstone.

10. `intern` means constructing a pool for objects. In Lox, it means any two strings with the same characters are the same objects. Thus, value equality is pointer equality.

11. Defining variables: push the expression operands into the `constants` array along with its operators; treat the variable name as a constant for memory efficiency. When we evaluate the chunk, we first get to see the constants definition. Once we meet the `OP_DEFINE_GLOBAL` directive, it's guaranteed our variable's values sits on the stack top. Same happens for `OP_SET_GLOBAL` and `OP_GET_GLOBAL`.

12. One problem without taking into consideration the precedence when calling `variable` or other expression functions is that we might end up with invalid assignment target. For example, suppose `a * b = a + b`, in the original code without `precedence` parameter, the code flows as the following:

    ```text
    declaration() -> statement() -> expressionStatement() -> expression();

    # Starting from `expression`, we enter `parseExpression` recursive calls.

    prefixFn()        # 'a', entering `variable`, emitting OP_GET_GLOBAL
    infixFn()         # '*', entering `binary`
    parseExpression() # with precedence = '*' + 1
    prefixFn()        # 'b', entering `variable`
    variable()        # '=', look ahead found '=', emit OP_SET_GLOBAL
    parseExpression() # with precedence = '=' + 1
    ...               # You know the rest
    ```

    Thus, we need to know at each stage whether `variable` can be used to parse assignment. To achieve this, we don't need the exact precedence context. A boolean value will do just fine.

13. Storing local variables: in jlox, we have a chain of environments(hashmaps) representing each scope, which is slow by nature. Consider a nested block like the following:

    ```
    var a = 1;
    {
        var b = 2;
        {
            var c = 3;
            var d = 4;
        }
        var e = 5;
    }
    ```

    When we design a language, we not only need to consider how to store a variable, but also take into account how to free it in case of memory leaks.

    The nested structure really simulates the behavior of a stack in that we push variables in the same scope in to the stack and pop them out when we're leaving the stack, in which process, we accomplish both tasks: storing and freeing.

    In `varDeclaration` function, if we hit upon a local variable, we skip the `identifierConstant` (storing the token's lexeme in the `constants` field of the current chunk) and variable declaration (emitting OP_DEFINE_GLOBAL). But we're still emitting the value operations (for example, OP_NIL for empty var declarations and OP_CONSTANT [index] otherwise). Also, we declare the local by adding it to the compiler's `locals` field so that we can keep track of it.

14. An if statement can only allow statement inside its then branch. A declaration right after if would be considered invalid since it's ambiguous what the value of this variable would be in other control branches.

15. In global scope, every statement ends up with an empty stack. One of the questions I had is about global variables. Then I realize global vars are stored in the vm's global hash table while our local variables live on the stack. Thus, when we're retrieving the local variables' names from the compiler's `locals` field, the index of which is exactly the index in vm's stack where it contains the value this local var corresponds to.

16. Conditional branch: JUMP_IF_FALSE, JUMP commands. A classic trick used in conditional branch is that we first emit jump (whether it be JUMP / JUMP_IF_FALSE) command with a 16-bit lookahead (size may vary) and record the position where we emit the jump. After we parse the rest of the source code (like the end of a block),  we calculate the offset (size) between the last recorded position and the current count of bytecode, with which we fill the 16-bit lookahead. One thing to keep in mind is that we need to emit pop command after we emit the jump since the conditional value evaluated at runtime in the stack needs to be cleaned.

17. The for loop is the trickiest part by far.