#include "env.h"
#include <stdlib.h>
#include "libraryinternal.h"
#include <stdio.h>
#include <string.h>

bool env_istrue_d(const char* name, bool _default) {
    const char* env = getenv(name);
    if(env == NULL) return _default;
    return *env == '1';
}

bool env_istrue(const char* name) {
    const char* env = getenv(name);
    return env != NULL && *env == '1';
}

size_t detect_device_memory_mb(void) {
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) return 2048;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long long mem_total;
        if (sscanf(line, "MemTotal: %llu kB", &mem_total) == 1) {
            fclose(fp);
            return (size_t)(mem_total / 1024);
        }
    }
    fclose(fp);
    return 2048;
}