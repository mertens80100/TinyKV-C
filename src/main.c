#include "kv_store.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *wal_path = argc > 1 ? argv[1] : "tinykv.wal";
    KVStore *store = kv_open(wal_path);
    if (!store) {
        fprintf(stderr, "Failed to open store\n");
        return 1;
    }

    char line[2048];
    while (fgets(line, sizeof(line), stdin)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        if (strcmp(line, "EXIT") == 0) {
            break;
        } else if (strcmp(line, "COUNT") == 0) {
            printf("%zu\n", kv_count(store));
        } else if (strcmp(line, "COMPACT") == 0) {
            printf("%s\n", kv_compact(store) == 0 ? "OK" : "ERR");
        } else if (strncmp(line, "SET ", 4) == 0) {
            char *key = line + 4;
            char *space = strchr(key, ' ');
            if (!space) {
                printf("ERR usage: SET <key> <value>\n");
            } else {
                *space = '\0';
                printf("%s\n", kv_set(store, key, space + 1) == 0 ? "OK" : "ERR");
            }
        } else if (strncmp(line, "GET ", 4) == 0) {
            const char *val = kv_get(store, line + 4);
            printf("%s\n", val ? val : "(nil)");
        } else if (strncmp(line, "DEL ", 4) == 0) {
            int rc = kv_del(store, line + 4);
            printf("%s\n", rc > 0 ? "OK" : "(nil)");
        } else {
            printf("ERR unknown command\n");
        }
    }

    kv_close(store);
    return 0;
}
