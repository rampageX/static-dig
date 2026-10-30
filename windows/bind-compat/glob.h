/* Filename globbing for the BIND configuration include directive on Win32.
 * BIND uses GLOB_ERR with a null errfunc and no append/offset options.
 * Windows FindFirstFile matches wildcard leaf names within one directory.
 */
#ifndef STATIC_DIG_WINDOWS_GLOB_H
#define STATIC_DIG_WINDOWS_GLOB_H

#include <winsock2.h>
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define GLOB_ERR 1
#define GLOB_NOSPACE 1
#define GLOB_ABORTED 2
#define GLOB_NOMATCH 3

typedef struct {
    size_t gl_pathc;
    char **gl_pathv;
} glob_t;

static inline void
globfree(glob_t *g) {
    size_t i;
    for (i = 0; i < g->gl_pathc; ++i) {
        free(g->gl_pathv[i]);
    }
    free(g->gl_pathv);
    g->gl_pathv = NULL;
    g->gl_pathc = 0;
}

static inline int
static_dig_glob_compare(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static inline int
glob(const char *pattern, int flags, int (*errfunc)(const char *, int),
     glob_t *g) {
    const char *slash = strrchr(pattern, '/');
    const char *backslash = strrchr(pattern, '\\');
    const char *leaf_separator;
    size_t prefix_len;
    HANDLE search;
    WIN32_FIND_DATAA found;
    int result = 0;

    (void)flags;
    (void)errfunc;
    g->gl_pathc = 0;
    g->gl_pathv = NULL;
    leaf_separator = slash == NULL ? backslash :
                     (backslash == NULL ? slash :
                      (slash > backslash ? slash : backslash));
    prefix_len = leaf_separator == NULL ? 0 :
                 (size_t)(leaf_separator - pattern + 1);

    search = FindFirstFileA(pattern, &found);
    if (search == INVALID_HANDLE_VALUE) {
        DWORD code = GetLastError();
        return (code == ERROR_FILE_NOT_FOUND ||
                code == ERROR_PATH_NOT_FOUND) ? GLOB_NOMATCH : GLOB_ABORTED;
    }

    do {
        size_t leaf_len = strlen(found.cFileName);
        char **new_names;
        char *name;
        if (strcmp(found.cFileName, ".") == 0 ||
            strcmp(found.cFileName, "..") == 0) continue;
        name = (char *)malloc(prefix_len + leaf_len + 1);
        if (name == NULL) { result = GLOB_NOSPACE; break; }
        memcpy(name, pattern, prefix_len);
        memcpy(name + prefix_len, found.cFileName, leaf_len + 1);
        new_names = (char **)realloc(g->gl_pathv,
                                    (g->gl_pathc + 1) * sizeof(char *));
        if (new_names == NULL) {
            free(name);
            result = GLOB_NOSPACE;
            break;
        }
        g->gl_pathv = new_names;
        g->gl_pathv[g->gl_pathc++] = name;
    } while (FindNextFileA(search, &found));
    FindClose(search);

    if (result != 0) { globfree(g); return result; }
    if (g->gl_pathc == 0) return GLOB_NOMATCH;
    qsort(g->gl_pathv, g->gl_pathc, sizeof(char *),
          static_dig_glob_compare);
    return 0;
}

#endif
