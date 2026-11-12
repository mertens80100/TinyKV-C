#ifndef KV_STORE_H
#define KV_STORE_H

#include <stddef.h>

typedef struct KVEntry {
    char *key;
    char *value;
    struct KVEntry *next;
} KVEntry;

typedef struct {
    KVEntry **buckets;
    size_t capacity;
    size_t count;
    char *wal_path;
} KVStore;

KVStore *kv_open(const char *wal_path);
void kv_close(KVStore *store);

int kv_set(KVStore *store, const char *key, const char *value);
const char *kv_get(const KVStore *store, const char *key);
int kv_del(KVStore *store, const char *key);
int kv_compact(KVStore *store);
size_t kv_count(const KVStore *store);

#endif
