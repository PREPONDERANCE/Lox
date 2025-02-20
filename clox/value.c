#include <stdio.h>
#include <string.h>

#include "value.h"
#include "memory.h"
#include "object.h"

bool valuesEqual(Value a, Value b)
{
    if (a.type != b.type)
        return false;

    switch (a.type)
    {
    case VAL_BOOL:
        return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
        return true;
    case VAL_NUMBER:
        return AS_NUMBER(a) == AS_NUMBER(b);
    // case VAL_OBJ:
    // {
    //     ObjString *s1 = AS_STRING(a);
    //     ObjString *s2 = AS_STRING(b);
    //     return s1->length == s2->length && memcmp(s1->chars, s2->chars, s1->length) == 0;
    // }

    // After string interning, address equality <=> value equality
    case VAL_OBJ:
        return AS_OBJ(a) == AS_OBJ(b);
    default:
        return false;
    }
}

void initValueArray(ValueArray *array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void freeValueArray(ValueArray *array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void writeValueArray(ValueArray *array, Value value)
{
    if (array->count + 1 > array->capacity)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count++] = value;
}

void printValue(Value value)
{
    switch (value.type)
    {
    case VAL_BOOL:
        printf(AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NIL:
        printf("nil");
        break;
    case VAL_NUMBER:
        printf("%g", AS_NUMBER(value));
        break;
    case VAL_OBJ:
        printObj(value);
        break;
    }
}