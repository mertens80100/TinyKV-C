#include "kv_store.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_crud_and_resize(void) {
    KVStore *s = kv_open(NULL);
    assert(s != NULL);
    assert(kv_count(s) == 0);

    char key[32], val[32];
    for (int i = 0; i < 64; i++) {
        snprintf(key, sizeof(key), "key_%d", i);
        snprintf(val, sizeof(val), "val_%d", i);
        assert(kv_set(s, key, val) == 0);
    }
    assert(kv_count(s) == 64);
    assert(strcmp(kv_get(s, "key_42"), "val_42") == 0);

    assert(kv_set(s, "key_42", "updated_42") == 0);
    assert(kv_count(s) == 64);
    assert(strcmp(kv_get(s, "key_42"), "updated_42") == 0);

    assert(kv_del(s, "key_42") == 1);
    assert(kv_get(s, "key_42") == NULL);
    assert(kv_del(s, "key_42") == 0);
    assert(kv_count(s) == 63);

    kv_close(s);
}

static void test_wal_recovery_and_compaction(void) {
    const char *wal = "test_recovery.wal";
    remove(wal);

    KVStore *s1 = kv_open(wal);
    assert(kv_set(s1, "lang", "C11") == 0);
    assert(kv_set(s1, "db", "WAL") == 0);
    assert(kv_set(s1, "temp", "123") == 0);
    assert(kv_del(s1, "temp") == 1);
    kv_close(s1);

    KVStore *s2 = kv_open(wal);
    assert(kv_count(s2) == 2);
    assert(strcmp(kv_get(s2, "lang"), "C11") == 0);
    assert(strcmp(kv_get(s2, "db"), "WAL") == 0);
    assert(kv_get(s2, "temp") == NULL);

    assert(kv_compact(s2) == 0);
    kv_close(s2);

    KVStore *s3 = kv_open(wal);
    assert(kv_count(s3) == 2);
    assert(strcmp(kv_get(s3, "lang"), "C11") == 0);
    kv_close(s3);

    remove(wal);
}

int main(void) {
    test_crud_and_resize();
    test_wal_recovery_and_compaction();
    printf("ALL TinyKV-C TESTS PASSED\n");
    return 0;
}
