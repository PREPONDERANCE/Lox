## Chapter 1

1. Allow instructions to have **operands**. For example, the `OP_CONSTANT` instruction is followed by a value indicating the index of the last inserted value. At evaluation time, the index will be read and processed.

2. A `do..while...` loop in the macro allows for a block of statements without the semicolons.

3. Prefix expression means we can judge what kind to expression this expression belongs to by just looking at the first token, while an infix expression means we can only determine the expression kind after consuming the first token.