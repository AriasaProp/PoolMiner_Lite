#ifndef JANSSON_H
#define JANSSON_H

#include <cstdio>

enum json_type {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_REAL,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL
};

struct json_t {
    json_type type;
    unsigned long refcount;
};

#define json_typeof(json)      ((json)->type)
#define json_is_object(json)   (json && json_typeof(json) == JSON_OBJECT)
#define json_is_array(json)    (json && json_typeof(json) == JSON_ARRAY)
#define json_is_string(json)   (json && json_typeof(json) == JSON_STRING)
#define json_is_integer(json)  (json && json_typeof(json) == JSON_INTEGER)
#define json_is_real(json)     (json && json_typeof(json) == JSON_REAL)
#define json_is_number(json)   (json_is_integer(json) || json_is_real(json))
#define json_is_true(json)     (json && json_typeof(json) == JSON_TRUE)
#define json_is_false(json)    (json && json_typeof(json) == JSON_FALSE)
#define json_is_boolean(json)  (json_is_true(json) || json_is_false(json))
#define json_is_null(json)     (json && json_typeof(json) == JSON_NULL)

/* construction, destruction, reference counting */

json_t *json_object(void);
json_t *json_array(void);
json_t *json_string(const char *);
json_t *json_string_nocheck(const char *);
json_t *json_integer(int);
json_t *json_real(double);
json_t *json_true(void);
json_t *json_false(void);
json_t *json_null(void);

inline json_t *json_incref(json_t *);
/* do not call json_delete directly */
void json_delete(json_t *);
inline void json_decref(json_t *);
/* getters, setters, manipulation */

unsigned int json_object_size(const json_t *);
json_t *json_object_get(const json_t *, const char *);
int json_object_set_new(json_t *, const char *, json_t *);
int json_object_set_new_nocheck(json_t *, const char *, json_t *);
int json_object_del(json_t *, const char *);
int json_object_clear(json_t *);
int json_object_update(json_t *, json_t *);
void *json_object_iter(json_t *);
void *json_object_iter_at(json_t *, const char *);
void *json_object_iter_next(json_t *, void *);
const char *json_object_iter_key(void *);
json_t *json_object_iter_value(void *);
int json_object_iter_set_new(json_t *, void *, json_t *);
inline int json_object_set(json_t *object, const char *, json_t *);
inline int json_object_set_nocheck(json_t *object, const char *, json_t *);
inline int json_object_iter_set(json_t *object, void *iter, json_t *);
unsigned int json_array_size(const json_t *);
json_t *json_array_get(const json_t *, unsigned int);
int json_array_set_new(json_t *, unsigned int, json_t *);
int json_array_append_new(json_t *, json_t *);
int json_array_insert_new(json_t *, unsigned int, json_t *);
int json_array_remove(json_t *, unsigned int);
int json_array_clear(json_t *);
int json_array_extend(json_t *, json_t *other);

inline int json_array_set(json_t *, unsigned int, json_t *);
inline int json_array_append(json_t *, json_t *);
inline int json_array_insert(json_t *, unsigned int, json_t *);

const char *json_string_value(const json_t *string);
int json_integer_value(const json_t *integer);
double json_real_value(const json_t *real);
double json_number_value(const json_t *);

int json_string_set(json_t *string, const char *);
int json_string_set_nocheck(json_t *string, const char *);
int json_integer_set(json_t *integer, int value);
int json_real_set(json_t *real, double value);


/* equality */

int json_equal(json_t *1, json_t *2);


/* copying */

json_t *json_copy(json_t *);
json_t *json_deep_copy(json_t *);


/* loading, printing */

#define JSON_ERROR_TEXT_LENGTH  160

struct json_error_t{
    char text[JSON_ERROR_TEXT_LENGTH];
    int line;
};

json_t *json_loads(const char *input, json_error_t *error);
json_t *json_loadf(FILE *input, json_error_t *error);
json_t *json_load_file(const char *path, json_error_t *error);

#define JSON_INDENT(n)      (n & 0xFF)
#define JSON_COMPACT        0x100
#define JSON_ENSURE_ASCII   0x200
#define JSON_SORT_KEYS      0x400
#define JSON_PRESERVE_ORDER 0x800

char *json_dumps(const json_t *, unsigned long);
int json_dumpf(const json_t *, FILE *, unsigned long);
int json_dump_file(const json_t *, const char *, unsigned long);

#endif

