#include <stdlib.h>
#include <string.h>

#include "table.h"
#include "value.h"
#include "object.h"
#include "memory.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table *table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table *table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

// Since we grow the hash table when load factor > TABLE_MAX_LOAD,
// it's guaranteed we won't run into infinite loops.
static Entry *findEntry(Entry *entries, int capacity, ObjString *key)
{
    uint32_t idx = key->hash % capacity;
    Entry *tombstone = NULL;

    for (;;)
    {
        Entry *entry = entries + idx;
        if (entry->key == NULL)
        {
            // Don't return the tombstone the moment we
            // hit it since the desired value maybe right
            // after it.
            if (IS_NIL(entry->value))
                return tombstone != NULL ? tombstone : entry;
            else
            {
                if (tombstone == NULL)
                    tombstone = entry;
            }
        }
        else if (entry->key == key)
            return entry;

        idx = (idx + 1) % capacity;
    }
}

// The reason why we don't use GROW_ARRAY is that all
// the hashed slot needs to be redistributed since the
// capacity changes. If we grow the array in place, we
// might run into a collision because the position is
// still hold by the old value.
static void adjustCapacity(Table *table, int capacity)
{
    Entry *entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // Recalculating the count, excluding the tombstones
    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        Entry *src = table->entries + i;
        if (src->key == NULL)
            continue;

        Entry *dst = findEntry(entries, capacity, src->key);
        dst->key = src->key;
        dst->value = src->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(Table *table, ObjString *key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry *target = findEntry(table->entries, table->capacity, key);
    bool isNew = target->key == NULL;
    // Only increment the count when the bucket
    // is a real emtpy slot rather than a tombstone.
    if (isNew && IS_NIL(target->value))
        table->count++;

    target->key = key;
    target->value = value;
    return isNew;
}

bool tableGet(Table *table, ObjString *key, Value *value)
{
    if (table->count == 0)
        return false;

    Entry *target = findEntry(table->entries, table->capacity, key);
    if (target->key == NULL)
        return false;

    *value = target->value;
    return true;
}

bool tableDelete(Table *table, ObjString *key)
{
    if (table->count == 0)
        return false;

    Entry *target = findEntry(table->entries, table->capacity, key);
    if (target->key == NULL)
        return false;

    target->key = NULL;
    target->value = BOOL_VAL(true);
    // We don't decrement the `table->count` here,
    // since tombstones are treated full buckets.
    // Otherwise, the array size may not grow and
    // we are kept inside an array filled with keys
    // and tombstones, running into an infinite loop
    // in `findEntry`.
    return true;
}

void tableAddAll(Table *from, Table *to)
{
    for (int i = 0; i < from->capacity; i++)
    {
        Entry *src = from->entries + i;
        if (src->key == NULL)
            continue;
        tableSet(to, src->key, src->value);
    }
}

/*
Params
------
- table: type Table*, the hash table pointer
- chars: type const char*, the C string to be found
- length: type int, length of the string
- hash: type uint32_t, hash value of `chars`
*/
ObjString *tableFindString(Table *table, const char *chars, int length, uint32_t hash)
{
    if (table->capacity == 0)
        return NULL;

    uint32_t idx = hash % table->capacity;
    for (;;)
    {
        Entry *entry = table->entries + idx;
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value))
                return NULL;
        }
        else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)

            return entry->key;

        idx = (idx + 1) % table->capacity;
    }
}