## Chapter 1

1. Allow instructions to have **operands**. For example, the `OP_CONSTANT` instruction is followed by a value indicating the index of the last inserted value. At evaluation time, the index will be read and processed.

2. A `do..while...` loop in the macro allows for a block of statements without the semicolons.

3. Prefix expression means we can judge what kind to expression this expression belongs to by just looking at the first token, while an infix expression means we can only determine the expression kind after consuming the first token.

4. `union` is a lot like `struct` except that the fields in `union` overlap with each other, meaning the memory usage in `union` equals the largest field.

5. `static void runtimeError(const char* format, ...)` means this function receives a varying number of arguments. `va_list args` initializes the arguments; `vfprintf(stderr, format, args)` works just like `fprintf` in that it takes in the file pointer (in this case, the stderr) and format string (for example, "%d %d %s") and the arguments. `va_end` is sort of like freeing the arguments.

6. The instructions our vm contains might not be aligned with the actual instructions. `a <= b` is equivalent to `!(a > b)`; `a != b` equivalent to that of `!(a == b)`.

7. When comparing the values, we cannot use `memcmp` to compare two value structs since the padding bits may differ.