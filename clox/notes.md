1. Allow instructions to have **operands**. For example, the `OP_CONSTANT` instruction is followed by a value indicating the index of the last inserted value. At evaluation time, the index will be read and processed.

2. A `do..while...` loop in the macro allows for a block of statements without the semicolons.

3. Prefix expression means we can judge what kind to expression this expression belongs to by just looking at the first token, while an infix expression means we can only determine the expression kind after consuming the first token.

4. `union` is a lot like `struct` except that the fields in `union` overlap with each other, meaning the memory usage in `union` equals the largest field.

5. `static void runtimeError(const char* format, ...)` means this function receives a varying number of arguments. `va_list args` initializes the arguments; `vfprintf(stderr, format, args)` works just like `fprintf` in that it takes in the file pointer (in this case, the stderr) and format string (for example, "%d %d %s") and the arguments. `va_end` is sort of like freeing the arguments.

6. The instructions our vm contains might not be aligned with the actual instructions. `a <= b` is equivalent to `!(a > b)`; `a != b` equivalent to that of `!(a == b)`.

7. When comparing the values, we cannot use `memcmp` to compare two value structs since the padding bits may differ.

8. Casting a pointer is really about changing the way the pointer looks at the memory. For example, a variable of type `char` named `c` is pointed by `char *` pointer `d`, namely `char* d = &c;`. `d` looks at only one byte of memory. But when we do `* (int *) d`, we are looking at 4 bytes of memory since `int` occupies 4 bytes.

9. The `tombstone` mechanism: suppose a hash table as `['a', 'b', 'c']`. All the elements' hash values are the same. If we delete entry 'b' entirely (key set to NULL, value set to NIL), we end up unable to find entry 'c' since the second slot is treated empty. As a result, on deleting entry 'b', we set the key to NULL but value to BOOL(true) (anything other than NIL) to mark this position as tombstone.

10. `intern` means constructing a pool for objects. In Lox, it means any two strings with the same characters are the same objects. Thus, value equality is pointer equality.