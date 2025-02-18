#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

typedef enum
{
    OBJ_STRING,
} ObjType;

struct Obj
{
    ObjType type;
    Obj *next;
    // An intrusive linked list for garbage collection
};

// This pattern mimics the behavior of OOP.
// The Obj struct expands inside ObjString struct.
// Thus by typecasting ObjString to Obj `(Obj *) str`,
// we are able to treat it as a generic Obj.
// And by downcasting this obj to ObjString, we restore
// it back to ObjString.
struct ObjString
{
    Obj obj;
    int length;
    char *chars;
};

// We define it as a standalone function because
// if otherwise and that we pass in the first argument
// as `pop()`, the pop method will be executed twice.
static inline bool isObjType(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

ObjString *takeString(char *chars, int length);
ObjString *copyString(const char *start, int length);
void printObj(Value value);

#endif
