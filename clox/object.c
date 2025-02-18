#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
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

static ObjString *allocateString(char *chars, int length)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->chars = chars;
    string->length = length;
    return string;
}

ObjString *takeString(char *chars, int length)
{
    return allocateString(chars, length);
}

ObjString *copyString(const char *start, int length)
{
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, start, length);
    chars[length] = '\0';
    return allocateString(chars, length);
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