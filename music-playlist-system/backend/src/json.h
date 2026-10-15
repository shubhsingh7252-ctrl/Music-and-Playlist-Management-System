/* json.h - minimal JSON reader for flat request bodies */
#ifndef JSON_H
#define JSON_H
#include <stddef.h>
int json_str(const char *js, const char *key, char *out, size_t cap);
int json_int(const char *js, const char *key, int *out);
int json_int_array(const char *js, const char *key, int *out, int max);   /* count or -1 */
#endif
