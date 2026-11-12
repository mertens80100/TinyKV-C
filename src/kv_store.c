#define _POSIX_C_SOURCE 200809L
#include "kv_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

static unsigned long fnv1a_hash(const char *str) {
    unsigned long hash = 14695981039346656037UL;
    while (*str) {
        hash ^= (unsigned char)(*str++);
        hash *= 1099511628211UL;
    }
    return hash;
}

static char *dup_str(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

static int is_valid_token(const char *s) {
    if (!s || *s == '\0') return 0;
    for (const char *p = s; *p; p++) {
        if (*p == '\n' || *p == '\r' || *p == '\t') return 0;
    }
    return 1;
}

static int kv_resize(KVStore *store, size_t new_capacity) {
    KVEntry **new_buckets = calloc(new_capacity, sizeof(KVEntry *));
    if (!new_buckets) return -1;

    for (size_t i = 0; i < store->capacity; i++) {
        KVEntry *curr = store->buckets[i];
        while (curr) {
            KVEntry *next = curr->next;
            size_t idx = fnv1a_hash(curr->key) % new_capacity;
            curr->next = new_buckets[idx];
            new_buckets[idx] = curr;
            curr = next;
        }
    }
    free(store->buckets);
    store->buckets = new_buckets;
    store->capacity = new_capacity;
    return 0;
}

static int kv_set_mem(KVStore *store, const char *key, const char *value) {
    if ((store->count + 1) * 10 >= store->capacity * 7) {
        if (kv_resize(store, store->capacity * 2) != 0) return -1;
    }
    size_t idx = fnv1a_hash(key) % store->capacity;
    KVEntry *curr = store->buckets[idx];
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            char *new_val = dup_str(value);
            if (!new_val) return -1;
            free(curr->value);
            curr->value = new_val;
            return 0;
        }
        curr = curr->next;
    }

    KVEntry *entry = malloc(sizeof(KVEntry));
    if (!entry) return -1;
    entry->key = dup_str(key);
    entry->value = dup_str(value);
    if (!entry->key || !entry->value) {
        free(entry->key);
        free(entry->value);
        free(entry);
        return -1;
    }
    entry->next = store->buckets[idx];
    store->buckets[idx] = entry;
    store->count++;
    return 0;
}

static int kv_del_mem(KVStore *store, const char *key) {
    size_t idx = fnv1a_hash(key) % store->capacity;
    KVEntry *curr = store->buckets[idx];
    KVEntry *prev = NULL;
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            if (prev) prev->next = curr->next;
            else store->buckets[idx] = curr->next;
            free(curr->key);
            free(curr->value);
            free(curr);
            store->count--;
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}

static int append_wal(const char *wal_path, const char *op, const char *key, const char *value) {
    if (!wal_path) return 0;
    FILE *fp = fopen(wal_path, "a");
    if (!fp) return -1;
    if (value) {
        fprintf(fp, "%s\t%s\t%s\n", op, key, value);
    } else {
        fprintf(fp, "%s\t%s\n", op, key);
    }
    fflush(fp);
    fclose(fp);
    return 0;
}

static void replay_wal(KVStore *store) {
    if (!store->wal_path) return;
    FILE *fp = fopen(store->wal_path, "r");
    if (!fp) return;

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (strncmp(line, "SET\t", 4) == 0) {
            char *key = line + 4;
            char *tab = strchr(key, '\t');
            if (tab) {
                *tab = '\0';
                kv_set_mem(store, key, tab + 1);
            }
        } else if (strncmp(line, "DEL\t", 4) == 0) {
            kv_del_mem(store, line + 4);
        }
    }
    fclose(fp);
}

KVStore *kv_open(const char *wal_path) {
    KVStore *store = malloc(sizeof(KVStore));
    if (!store) return NULL;
    store->capacity = INITIAL_CAPACITY;
    store->count = 0;
    store->buckets = calloc(store->capacity, sizeof(KVEntry *));
    store->wal_path = wal_path ? dup_str(wal_path) : NULL;
    if (!store->buckets || (wal_path && !store->wal_path)) {
        free(store->buckets);
        free(store->wal_path);
        free(store);
        return NULL;
    }
    replay_wal(store);
    return store;
}

void kv_close(KVStore *store) {
    if (!store) return;
    for (size_t i = 0; i < store->capacity; i++) {
        KVEntry *curr = store->buckets[i];
        while (curr) {
            KVEntry *next = curr->next;
            free(curr->key);
            free(curr->value);
            free(curr);
            curr = next;
        }
    }
    free(store->buckets);
    free(store->wal_path);
    free(store);
}

int kv_set(KVStore *store, const char *key, const char *value) {
    if (!store || !is_valid_token(key) || !is_valid_token(value)) return -1;
    if (kv_set_mem(store, key, value) != 0) return -1;
    return append_wal(store->wal_path, "SET", key, value);
}

const char *kv_get(const KVStore *store, const char *key) {
    if (!store || !key) return NULL;
    size_t idx = fnv1a_hash(key) % store->capacity;
    KVEntry *curr = store->buckets[idx];
    while (curr) {
        if (strcmp(curr->key, key) == 0) return curr->value;
        curr = curr->next;
    }
    return NULL;
}

int kv_del(KVStore *store, const char *key) {
    if (!store || !is_valid_token(key)) return -1;
    int removed = kv_del_mem(store, key);
    if (removed > 0) {
        if (append_wal(store->wal_path, "DEL", key, NULL) != 0) return -1;
    }
    return removed;
}

int kv_compact(KVStore *store) {
    if (!store || !store->wal_path) return 0;
    size_t tmp_len = strlen(store->wal_path) + 5;
    char *tmp_path = malloc(tmp_len);
    if (!tmp_path) return -1;
    snprintf(tmp_path, tmp_len, "%s.tmp", store->wal_path);

    FILE *fp = fopen(tmp_path, "w");
    if (!fp) {
        free(tmp_path);
        return -1;
    }
    for (size_t i = 0; i < store->capacity; i++) {
        KVEntry *curr = store->buckets[i];
        while (curr) {
            fprintf(fp, "SET\t%s\t%s\n", curr->key, curr->value);
            curr = curr->next;
        }
    }
    fclose(fp);
    if (rename(tmp_path, store->wal_path) != 0) {
        free(tmp_path);
        return -1;
    }
    free(tmp_path);
    return 0;
}

size_t kv_count(const KVStore *store) {
    return store ? store->count : 0;
}
