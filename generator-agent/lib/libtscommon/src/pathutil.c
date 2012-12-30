/*
 * fnutil.c
 *
 *  Created on: 29.12.2012
 *      Author: myaut
 */

#include <pathutil.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#if defined(PLAT_WIN)
const char* path_separator = "\\";
const int psep_length = 1;

#elif defined(PLAT_POSIX)
const char* path_separator = "/";
const int psep_length = 1;

#else
#error "Unknown path separator for this platform"

#endif

/**
 * Create path from array into string. Similiar to python's os.path.join
 *
 * @param dest Destination string
 * @param len Length of destination string
 * @param num_parts Number of parts provided
 * @param parts Parts of path
 *
 * @return destination with formed path or NULL if destination was overflown
 *  */
char* path_join_array(char* dest, size_t len, int num_parts, char** parts) {
    char* part = parts[0];
    int i = 0;
    int idx = 0;
    size_t part_len = 0;

    dest[0] = '\0';

    while(part != NULL && i < num_parts) {
        part_len = strlen(part);
        if(part_len > len)
            return NULL;

        len -= part_len;

        /* If previous part was not finished by separator,
         * add it (works only if psep_length == 1 */
        if(i != 0 && dest[idx - 1] != *path_separator) {
            len -= psep_length;
            idx += psep_length;
            strncat(dest, path_separator, len);
        }

        strncat(dest, part, len);
        idx += part_len;

        part = parts[++i];
    }

    return dest;
}

/**
 * Create path from various arguments
 * @note Last argument should be always NULL
 *
 * Example: path_join(path, PATHMAXLEN, "/tmp", tmpdir, "file", NULL)
 *
 * @see path_join_array
 */
char* path_join(char* dest, size_t len, ...) {
    char* parts[PATHMAXPARTS];
    char* part = NULL;
    va_list va;
    int i = 0;

    va_start(va, len);
    do {
        part = va_arg(va, char*);
        parts[i++] = part;

        /*Too much parts*/
        if(i == PATHMAXPARTS)
            return NULL;
    } while(part != NULL);
    va_end(va);

    return path_join_array(dest, len, i, parts);
}

static void path_split_add(path_split_iter_t* iter, const char* orig, const char* part) {
    size_t len = part - orig;

    if(len == 0)
        return;

    strncpy(iter->ps_dest, orig, len);

    iter->ps_parts[iter->ps_num_parts++] = iter->ps_dest;

    iter->ps_dest += len;
    *iter->ps_dest++ = 0;
}

static const char* strrchr_x(const char* s, const char* from, int c) {
    do {
        if(from == s)
            return NULL;

        --from;
    } while(*from != c);

    return from;
}

/**
 * Split path into parts. Saves result into path iterator which may be walked using
 * path_split_next. Returns first part of path or NULL if max is too large.
 *
 * If max negative, path_split would split reversely, so first item would be basename
 *
 * @param iter Allocated split path iterator
 * @param max Maximum parts which will be processed.
 *
 * @return First path part
 * */
char* path_split(path_split_iter_t* iter, int max, const char* path) {
    const char *part, *orig;
    char* dest = iter->ps_storage;

    long max_parts = abs(max);

    size_t part_len = 0;

    iter->ps_num_parts = 0;
    iter->ps_part = 1;

    iter->ps_dest = iter->ps_storage;

    /* Invalid max value*/
    if(max_parts == 0 || max_parts > PATHMAXPARTS)
        return NULL;

    /* for psep_length != 1 should use strstr or reverse analogue */
    if(max < 0) {
        /* Reverse - search */
        part = orig = path + strlen(path);

        while((part = strrchr_x(path, part, *path_separator)) != NULL) {
            if(++max == 0)
                break;

            path_split_add(iter, part + psep_length, orig);

            orig = part;
        }

        if(orig != path) {
            /* Ignore trailing separators */
            part = path;
            while(*part == *path_separator) ++part;

            path_split_add(iter, part, orig);
        }
    }
    else {
        path = orig = path;

        while((part = strchr(part, *path_separator)) != NULL) {
            if(max-- == 0)
                break;

            path_split_add(iter, orig, part);

            orig = part + psep_length;
            part += psep_length;
        }

        path_split_add(iter, orig, path + strlen(path));
    }

    return iter->ps_parts[0];
}

/**
 * @return next path part from iterator
 * */
char* path_split_next(path_split_iter_t* iter) {
    if(iter->ps_part == iter->ps_num_parts)
        return NULL;

    return iter->ps_parts[iter->ps_part++];
}
