#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objType) \
    (type *)allocateObj(sizeof(type), objType)

static Obj *allocateObj(size_t size, ObjType objType)
{
    Obj *obj = (Obj *)reallocate(NULL, 0, size);
    obj->type = objType;

    // Track all the objects allocated
    obj->next = vm.objects;
    vm.objects = obj;
    return obj;
}

static ObjString *allocateString(char *chars, int length, uint32_t hash)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);

    string->chars = chars;
    string->length = length;
    string->hash = hash;
    tableSet(&vm.strings, string, NIL_VAL);
    // Whenever we allocate a new string, we intern it.

    return string;
}

// FNV-1a hash function
static uint32_t hashString(const char *key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString *takeString(char *chars, int length)
{
    // Before taking the string, check to see
    // if there're any interned ones.
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);

    if (interned != NULL)
    {
        // Remember we use this API when concatenating
        // strings back in vm.c without freeing it.
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}

ObjString *copyString(const char *start, int length)
{
    // Before copy the string, check if there're
    // any interned ones.
    uint32_t hash = hashString(start, length);
    ObjString *interned = tableFindString(&vm.strings, start, length, hash);

    if (interned != NULL)
        return interned;

    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, start, length);
    chars[length] = '\0';
    return allocateString(chars, length, hash);
}

void printObj(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}