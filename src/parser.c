#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_LINKS_INITIAL_CAP 32

static int starts_with_ci(const char* s, const char* prefix)
{
    return strncasecmp(s, prefix, strlen(prefix)) == 0;
}

/* Prenosivi zamena za strcasestr (nije deo C99/POSIX standarda svuda). */
static const char* find_ci(const char* haystack, const char* needle)
{
    size_t needle_len = strlen(needle);
    if (needle_len == 0) {
        return haystack;
    }
    for (const char* p = haystack; *p != '\0'; p++) {
        if (strncasecmp(p, needle, needle_len) == 0) {
            return p;
        }
    }
    return NULL;
}

static void get_scheme(const char* base, char* out, size_t out_size)
{
    const char* p = strstr(base, "://");
    size_t len = p != NULL ? (size_t)(p - base) : 0;
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, base, len);
    out[len] = '\0';
}

static void get_scheme_host(const char* base, char* out, size_t out_size)
{
    const char* p = strstr(base, "://");
    if (p == NULL) {
        snprintf(out, out_size, "%s", base);
        return;
    }
    const char* host_start = p + 3;
    const char* slash = strchr(host_start, '/');
    size_t len = slash != NULL ? (size_t)(slash - base) : strlen(base);
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, base, len);
    out[len] = '\0';
}

static void get_base_dir(const char* base, char* out, size_t out_size)
{
    const char* p = strstr(base, "://");
    const char* search_start = p != NULL ? p + 3 : base;
    const char* last_slash = strrchr(search_start, '/');

    if (last_slash == NULL) {
        snprintf(out, out_size, "%s/", base);
        return;
    }

    size_t len = (size_t)(last_slash - base) + 1;
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, base, len);
    out[len] = '\0';
}

int parser_resolve_url(const char* base_url, const char* link, char* out, size_t out_size)
{
    /* preskoči vodeći/prateći whitespace */
    while (*link == ' ' || *link == '\t' || *link == '\n' || *link == '\r') {
        link++;
    }
    size_t len = strlen(link);
    while (len > 0 && (link[len - 1] == ' ' || link[len - 1] == '\t'
                           || link[len - 1] == '\n' || link[len - 1] == '\r')) {
        len--;
    }
    if (len == 0 || len >= 2048) {
        return 0;
    }
    char trimmed[2048];
    memcpy(trimmed, link, len);
    trimmed[len] = '\0';

    if (trimmed[0] == '#' || trimmed[0] == '\0') {
        return 0; /* čist fragment */
    }
    if (starts_with_ci(trimmed, "mailto:") || starts_with_ci(trimmed, "javascript:")
        || starts_with_ci(trimmed, "tel:") || starts_with_ci(trimmed, "data:")) {
        return 0;
    }

    if (starts_with_ci(trimmed, "http://") || starts_with_ci(trimmed, "https://")) {
        snprintf(out, out_size, "%s", trimmed);
        return 1;
    }

    if (strstr(trimmed, "://") != NULL) {
        return 0; /* nepodržana šema (ftp:, ws:, ...) */
    }

    if (starts_with_ci(trimmed, "//")) {
        char scheme[16];
        get_scheme(base_url, scheme, sizeof(scheme));
        snprintf(out, out_size, "%s:%s", scheme, trimmed);
        return 1;
    }

    if (trimmed[0] == '/') {
        char scheme_host[512];
        get_scheme_host(base_url, scheme_host, sizeof(scheme_host));
        snprintf(out, out_size, "%s%s", scheme_host, trimmed);
        return 1;
    }

    char base_dir[1024];
    get_base_dir(base_url, base_dir, sizeof(base_dir));
    snprintf(out, out_size, "%s%s", base_dir, trimmed);
    return 1;
}

static int append_link(char*** links, size_t* count, size_t* cap, const char* url)
{
    if (*count == *cap) {
        size_t new_cap = (*cap == 0) ? MAX_LINKS_INITIAL_CAP : (*cap * 2);
        char** new_arr = (char**)realloc(*links, new_cap * sizeof(char*));
        if (new_arr == NULL) {
            return -1;
        }
        *links = new_arr;
        *cap = new_cap;
    }

    char* copy = strdup(url);
    if (copy == NULL) {
        return -1;
    }
    (*links)[*count] = copy;
    (*count)++;
    return 0;
}

int parser_extract_links(const char* html, const char* base_url,
    char*** out_links, size_t* out_count)
{
    char** links = NULL;
    size_t count = 0;
    size_t cap = 0;

    const char* p = html;
    while ((p = find_ci(p, "href")) != NULL) {
        const char* cursor = p + 4;

        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r') {
            cursor++;
        }
        if (*cursor != '=') {
            p = p + 4;
            continue;
        }
        cursor++;
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r') {
            cursor++;
        }

        char quote = *cursor;
        if (quote != '"' && quote != '\'') {
            p = p + 4;
            continue;
        }
        cursor++;

        const char* value_start = cursor;
        const char* end_quote = strchr(cursor, quote);
        if (end_quote == NULL) {
            break;
        }

        size_t value_len = (size_t)(end_quote - value_start);
        if (value_len > 0 && value_len < 2048) {
            char value[2048];
            memcpy(value, value_start, value_len);
            value[value_len] = '\0';

            char resolved[2048];
            if (parser_resolve_url(base_url, value, resolved, sizeof(resolved))) {
                if (append_link(&links, &count, &cap, resolved) != 0) {
                    parser_free_links(links, count);
                    return -1;
                }
            }
        }

        p = end_quote + 1;
    }

    *out_links = links;
    *out_count = count;
    return 0;
}

void parser_free_links(char** links, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        free(links[i]);
    }
    free(links);
}
