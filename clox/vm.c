#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

VM vm;

static void resetStack()
{
    vm.stackTop = vm.stack;
}

static void runtimeError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);

    int instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM()
{
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);
}

void freeVM()
{
    freeObjects();
    freeTable(&vm.strings);
    freeTable(&vm.globals);
}

void push(Value value)
{
    *vm.stackTop++ = value;
}

Value pop()
{
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString *s2 = AS_STRING(pop());
    ObjString *s1 = AS_STRING(pop());

    int length = s1->length + s2->length;
    char *final = ALLOCATE(char, length + 1);
    memcpy(final, s1->chars, s1->length);
    memcpy(final + s1->length, s2->chars, s2->length);
    final[length] = '\0';

    ObjString *str = takeString(final, length);
    push(OBJ_VAL(str));
}

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() (AS_STRING(READ_CONSTANT()))
#define BINARY_OP(valueType, op)                            \
    do                                                      \
    {                                                       \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)))     \
        {                                                   \
            runtimeError("Operands must both be numbers."); \
            return INTERPRET_RUNTIME_ERROR;                 \
        }                                                   \
        double b = AS_NUMBER(pop());                        \
        double a = AS_NUMBER(pop());                        \
        push(valueType(a op b));                            \
    } while (false)

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION

        printf("[STACK ] [");
        for (Value *slot = vm.stack; slot < vm.stackTop; slot++)
        {
            printValue(*slot);
            if (slot + 1 != vm.stackTop)
                printf(", ");
        }
        printf("]\n");

        printf("[OPCODE] ");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        switch (READ_BYTE())
        {
        case OP_CONSTANT:
        {
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OP_NIL:
            push(NIL_VAL);
            break;
        case OP_TRUE:
            push(BOOL_VAL(true));
            break;
        case OP_FALSE:
            push(BOOL_VAL(false));
            break;
        case OP_POP:
            pop();
            break;
        case OP_DEFINE_GLOBAL:
        {
            ObjString *name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));
            pop();
            // Note that we don’t pop the value until after
            // we add it to the hash table. That ensures the
            // VM can still find the value if a garbage collection
            // is triggered right in the middle of adding it to the
            // hash table due to possible dynamic reallocation
            break;
        }
        case OP_GET_GLOBAL:
        {
            ObjString *name = READ_STRING();
            Value value;
            if (!tableGet(&vm.globals, name, &value))
            {
                runtimeError("Undefined variable %s.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_SET_GLOBAL:
        {
            ObjString *name = READ_STRING();
            // Do not pop the value out. We're just
            // reassign the target variable.
            // Los doesn't do implicit declaration.
            if (tableSet(&vm.globals, name, peek(0)))
            {
                // If tableSet returns true, indicating
                // we didn't find a variable in the existing
                // one, report an error.
                tableDelete(&vm.globals, name);
                runtimeError("Undefined variable %s.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_GET_LOCAL:
        {
            // Push the value again to the stack seems redundant
            // but every other operations relies on the peek element.
            // Suppose we're to print a local var without re-push
            // the value and that the cur stack size is 1, we end up
            // losing the original local var. Remember, local vars
            // are not looked up by name, they live inside the stack.

            uint8_t slot = READ_BYTE();
            push(vm.stack[slot]);
            break;
        }
        case OP_SET_LOCAL:
        {
            // We do not perform pop here since assignment itself is
            // an expression and every expression ends up with a pop
            // command.

            uint8_t slot = READ_BYTE();
            vm.stack[slot] = peek(0);
            break;
        }
        case OP_EQUAL:
        {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
            break;
        }
        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);
            break;
        case OP_ADD:
        {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
                concatenate();
            else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
            {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            }
            else
            {
                runtimeError("Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_NOT:
            push(BOOL_VAL(isFalsey(pop())));
            break;
        case OP_NEGATE:
            if (!IS_NUMBER(peek(0)))
            {
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break;
        case OP_PRINT:
            printValue(pop());
            printf("\n");
            break;
        case OP_RETURN:
            return INTERPRET_OK;
        }
    }

#undef BINARY_OP
#undef READ_STRING
#undef READ_CONSTANT
#undef READ_BYTE // Remove all defined macros
}

InterpretResult interpret(const char *source)
{
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk))
    {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = chunk.code;

    InterpretResult res = run();
    freeChunk(&chunk);

    return res;
}