#include "jansson.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <climits>
#include <cstdarg>
#include <ctype.h>
#include <cerrno>

/* hashtable */ 
typedef unsigned int (*key_hash_fn)(const void *key);
typedef int (*key_cmp_fn)(const void *key1, const void *key2);
typedef void (*free_fn)(void *key);

struct hashtable_list {
    struct hashtable_list *prev;
    struct hashtable_list *next;
};

struct hashtable_pair {
    void *key;
    void *value;
    unsigned int hash;
    struct hashtable_list list;
};

struct hashtable_bucket {
    struct hashtable_list *first;
    struct hashtable_list *last;
};

struct hashtable_t {
    unsigned int size;
    struct hashtable_bucket *buckets;
    unsigned int num_buckets;  /* index to primes[] */
    struct hashtable_list list;

    key_hash_fn hash_key;
    key_cmp_fn cmp_keys;  /* returns non-zero for equal keys */
    free_fn free_key;
    free_fn free_value;
};

#define container_of(ptr_, type_, member_)                      \
    ((type_ *)((char *)ptr_ - (size_t)&((type_ *)0)->member_))

#define list_to_pair(list_)  container_of(list_, hashtable_pair, list)

static inline void list_init(hashtable_list *list)
{
    list->next = list;
    list->prev = list;
}

static inline void list_insert(hashtable_list *list, hashtable_list *node)
{
    node->next = list;
    node->prev = list->prev;
    list->prev->next = node;
    list->prev = node;
}

static inline void list_remove(hashtable_list *list)
{
    list->prev->next = list->next;
    list->next->prev = list->prev;
}

static inline int bucket_is_empty(hashtable_t *hashtable, hashtable_bucket *bucket)
{
    return bucket->first == &hashtable->list && bucket->first == bucket->last;
}

static void insert_to_bucket(hashtable_t *hashtable, hashtable_bucket *bucket,
                             hashtable_list *list)
{
    if(bucket_is_empty(hashtable, bucket))
    {
        list_insert(&hashtable->list, list);
        bucket->first = bucket->last = list;
    }
    else
    {
        list_insert(bucket->first, list);
        bucket->first = list;
    }
}

static unsigned int primes[] = {
    5, 13, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
    49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469,
    12582917, 25165843, 50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741
};
static const unsigned int num_primes = sizeof(primes) / sizeof(unsigned int);

static inline unsigned int num_buckets(hashtable_t *hashtable)
{
    return primes[hashtable->num_buckets];
}


static hashtable_pair *hashtable_find_pair(hashtable_t *hashtable, hashtable_bucket *bucket,
                                   const void *key, unsigned int hash)
{
    hashtable_list *list;
    hashtable_pair *pair;

    if(bucket_is_empty(hashtable, bucket))
        return NULL;

    list = bucket->first;
    while(1)
    {
        pair = list_to_pair(list);
        if(pair->hash == hash && hashtable->cmp_keys(pair->key, key))
            return pair;

        if(list == bucket->last)
            break;

        list = list->next;
    }

    return NULL;
}

/* returns 0 on success, -1 if key was not found */
static int hashtable_do_del(hashtable_t *hashtable,
                            const void *key, unsigned int hash)
{
    hashtable_pair *pair;
    hashtable_bucket *bucket;
    unsigned int index;

    index = hash % num_buckets(hashtable);
    bucket = &hashtable->buckets[index];

    pair = hashtable_find_pair(hashtable, bucket, key, hash);
    if(!pair)
        return -1;

    if(&pair->list == bucket->first && &pair->list == bucket->last)
        bucket->first = bucket->last = &hashtable->list;

    else if(&pair->list == bucket->first)
        bucket->first = pair->list.next;

    else if(&pair->list == bucket->last)
        bucket->last = pair->list.prev;

    list_remove(&pair->list);

    if(hashtable->free_key)
        hashtable->free_key(pair->key);
    if(hashtable->free_value)
        hashtable->free_value(pair->value);

    free(pair);
    hashtable->size--;

    return 0;
}

static void hashtable_do_clear(hashtable_t *hashtable)
{
    hashtable_list *list, *next;
    hashtable_pair *pair;

    for(list = hashtable->list.next; list != &hashtable->list; list = next)
    {
        next = list->next;
        pair = list_to_pair(list);
        if(hashtable->free_key)
            hashtable->free_key(pair->key);
        if(hashtable->free_value)
            hashtable->free_value(pair->value);
        free(pair);
    }
}

static int hashtable_do_rehash(hashtable_t *hashtable)
{
    hashtable_list *list, *next;
    hashtable_pair *pair;
    unsigned int i, index, new_size;

    free(hashtable->buckets);

    hashtable->num_buckets++;
    new_size = num_buckets(hashtable);

    hashtable->buckets = malloc(new_size * sizeof(hashtable_bucket));
    if(!hashtable->buckets)
        return -1;

    for(i = 0; i < num_buckets(hashtable); i++)
    {
        hashtable->buckets[i].first = hashtable->buckets[i].last =
            &hashtable->list;
    }

    list = hashtable->list.next;
    list_init(&hashtable->list);

    for(; list != &hashtable->list; list = next) {
        next = list->next;
        pair = list_to_pair(list);
        index = pair->hash % new_size;
        insert_to_bucket(hashtable, &hashtable->buckets[index], &pair->list);
    }

    return 0;
}


hashtable_t *hashtable_create(key_hash_fn hash_key, key_cmp_fn cmp_keys,
                              free_fn free_key, free_fn free_value)
{
    hashtable_t *hashtable = malloc(sizeof(hashtable_t));
    if(!hashtable)
        return NULL;

    if(hashtable_init(hashtable, hash_key, cmp_keys, free_key, free_value))
    {
        free(hashtable);
        return NULL;
    }

    return hashtable;
}

void hashtable_destroy(hashtable_t *hashtable)
{
    hashtable_close(hashtable);
    free(hashtable);
}

int hashtable_init(hashtable_t *hashtable,
                   key_hash_fn hash_key, key_cmp_fn cmp_keys,
                   free_fn free_key, free_fn free_value)
{
    unsigned int i;

    hashtable->size = 0;
    hashtable->num_buckets = 0;  /* index to primes[] */
    hashtable->buckets = malloc(num_buckets(hashtable) * sizeof(hashtable_bucket));
    if(!hashtable->buckets)
        return -1;

    list_init(&hashtable->list);

    hashtable->hash_key = hash_key;
    hashtable->cmp_keys = cmp_keys;
    hashtable->free_key = free_key;
    hashtable->free_value = free_value;

    for(i = 0; i < num_buckets(hashtable); i++)
    {
        hashtable->buckets[i].first = hashtable->buckets[i].last =
            &hashtable->list;
    }

    return 0;
}

void hashtable_close(hashtable_t *hashtable)
{
    hashtable_do_clear(hashtable);
    free(hashtable->buckets);
}

int hashtable_set(hashtable_t *hashtable, void *key, void *value)
{
    hashtable_pair *pair;
    hashtable_bucket *bucket;
    unsigned int hash, index;

    /* rehash if the load ratio exceeds 1 */
    if(hashtable->size >= num_buckets(hashtable))
        if(hashtable_do_rehash(hashtable))
            return -1;

    hash = hashtable->hash_key(key);
    index = hash % num_buckets(hashtable);
    bucket = &hashtable->buckets[index];
    pair = hashtable_find_pair(hashtable, bucket, key, hash);

    if(pair)
    {
        if(hashtable->free_key)
            hashtable->free_key(key);
        if(hashtable->free_value)
            hashtable->free_value(pair->value);
        pair->value = value;
    }
    else
    {
        pair = malloc(sizeof(hashtable_pair));
        if(!pair)
            return -1;

        pair->key = key;
        pair->value = value;
        pair->hash = hash;
        list_init(&pair->list);

        insert_to_bucket(hashtable, bucket, &pair->list);

        hashtable->size++;
    }
    return 0;
}

void *hashtable_get(hashtable_t *hashtable, const void *key)
{
    hashtable_pair *pair;
    unsigned int hash;
    hashtable_bucket *bucket;

    hash = hashtable->hash_key(key);
    bucket = &hashtable->buckets[hash % num_buckets(hashtable)];

    pair = hashtable_find_pair(hashtable, bucket, key, hash);
    if(!pair)
        return NULL;

    return pair->value;
}

int hashtable_del(hashtable_t *hashtable, const void *key)
{
    unsigned int hash = hashtable->hash_key(key);
    return hashtable_do_del(hashtable, key, hash);
}

void hashtable_clear(hashtable_t *hashtable)
{
    unsigned int i;

    hashtable_do_clear(hashtable);

    for(i = 0; i < num_buckets(hashtable); i++)
    {
        hashtable->buckets[i].first = hashtable->buckets[i].last =
            &hashtable->list;
    }

    list_init(&hashtable->list);
    hashtable->size = 0;
}

void *hashtable_iter(hashtable_t *hashtable)
{
    return hashtable_iter_next(hashtable, &hashtable->list);
}

void *hashtable_iter_at(hashtable_t *hashtable, const void *key)
{
    hashtable_pair *pair;
    unsigned int hash;
    hashtable_bucket *bucket;

    hash = hashtable->hash_key(key);
    bucket = &hashtable->buckets[hash % num_buckets(hashtable)];

    pair = hashtable_find_pair(hashtable, bucket, key, hash);
    if(!pair)
        return NULL;

    return &pair->list;
}

void *hashtable_iter_next(hashtable_t *hashtable, void *iter)
{
    hashtable_list *list = (hashtable_list *)iter;
    if(list->next == &hashtable->list)
        return NULL;
    return list->next;
}

void *hashtable_iter_key(void *iter)
{
    hashtable_pair *pair = list_to_pair((hashtable_list *)iter);
    return pair->key;
}

void *hashtable_iter_value(void *iter)
{
    hashtable_pair *pair = list_to_pair((hashtable_list *)iter);
    return pair->value;
}

void hashtable_iter_set(hashtable_t *hashtable, void *iter, void *value)
{
    hashtable_pair *pair = list_to_pair((hashtable_list *)iter);

    if(hashtable->free_value)
        hashtable->free_value(pair->value);

    pair->value = value;
}

/* hashtable end */ 

/* jansson_private.h */

#define container_of(ptr_, type_, member_) ((type_ *)((char *)ptr_ - (size_t)&((type_ *)0)->member_))

struct json_object_t {
  json_t json;
  hashtable_t hashtable;
  unsigned long serial;
  int visited;
};
struct json_array_t {
  json_t json;
  unsigned int size;
  unsigned int entries;
  json_t **table;
  int visited;
};
struct json_string_t {
  json_t json;
  char *value;
};
struct json_real_t {
  json_t json;
  double value;
};
struct json_integer_t {
  json_t json;
  int value;
};

#define json_to_object(json_)  container_of(json_, json_object_t, json)
#define json_to_array(json_)   container_of(json_, json_array_t, json)
#define json_to_string(json_)  container_of(json_, json_string_t, json)
#define json_to_real(json_)   container_of(json_, json_real_t, json)
#define json_to_integer(json_) container_of(json_, json_integer_t, json)

struct object_key_t {
  unsigned long serial;
  char key[];
};

const object_key_t *jsonp_object_iter_fullkey(void *iter);

/* jansson_private.h end */
/* util */
#define max(a, b)  ((a) > (b) ? (a) : (b))
/* util end */

/* strbuffer */
struct strbuffer_t{
    char *value;
    int length;   /* bytes used */
    int size;     /* bytes allocated */
};

#define STRBUFFER_MIN_SIZE  16
#define STRBUFFER_FACTOR    2

int strbuffer_init(strbuffer_t *strbuff){
    strbuff->size = STRBUFFER_MIN_SIZE;
    strbuff->length = 0;

    strbuff->value = malloc(strbuff->size);
    if(!strbuff->value)
        return -1;

    /* initialize to empty */
    strbuff->value[0] = '\0';
    return 0;
}
void strbuffer_close(strbuffer_t *strbuff){
    free(strbuff->value);
    strbuff->size = 0;
    strbuff->length = 0;
    strbuff->value = NULL;
}
void strbuffer_clear(strbuffer_t *strbuff){
    strbuff->length = 0;
    strbuff->value[0] = '\0';
}
const char *strbuffer_value(const strbuffer_t *strbuff){
    return strbuff->value;
}
char *strbuffer_steal_value(strbuffer_t *strbuff){
    char *result = strbuff->value;
    strbuffer_init(strbuff);
    return result;
}
int strbuffer_append(strbuffer_t *strbuff, const char *string){
    return strbuffer_append_bytes(strbuff, string, strlen(string));
}
int strbuffer_append_byte(strbuffer_t *strbuff, char byte){
    return strbuffer_append_bytes(strbuff, &byte, 1);
}
int strbuffer_append_bytes(strbuffer_t *strbuff, const char *data, int size){
    if(strbuff->length + size >= strbuff->size)
    {
        strbuff->size = max(strbuff->size * STRBUFFER_FACTOR,
                            strbuff->length + size + 1);

        strbuff->value = realloc(strbuff->value, strbuff->size);
        if(!strbuff->value)
            return -1;
    }

    memcpy(strbuff->value + strbuff->length, data, size);
    strbuff->length += size;
    strbuff->value[strbuff->length] = '\0';

    return 0;
}
char strbuffer_pop(strbuffer_t *strbuff){
    if(strbuff->length > 0) {
        char c = strbuff->value[--strbuff->length];
        strbuff->value[strbuff->length] = '\0';
        return c;
    }
    else
        return '\0';
}

/* strbuffer end */


/* utf */

#include <cinttypes>

int utf8_encode(int32_t codepoint, char *buffer, int *size){
    if(codepoint < 0)
        return -1;
    else if(codepoint < 0x80)
    {
        buffer[0] = (char)codepoint;
        *size = 1;
    }
    else if(codepoint < 0x800)
    {
        buffer[0] = 0xC0 + ((codepoint & 0x7C0) >> 6);
        buffer[1] = 0x80 + ((codepoint & 0x03F));
        *size = 2;
    }
    else if(codepoint < 0x10000)
    {
        buffer[0] = 0xE0 + ((codepoint & 0xF000) >> 12);
        buffer[1] = 0x80 + ((codepoint & 0x0FC0) >> 6);
        buffer[2] = 0x80 + ((codepoint & 0x003F));
        *size = 3;
    }
    else if(codepoint <= 0x10FFFF)
    {
        buffer[0] = 0xF0 + ((codepoint & 0x1C0000) >> 18);
        buffer[1] = 0x80 + ((codepoint & 0x03F000) >> 12);
        buffer[2] = 0x80 + ((codepoint & 0x000FC0) >> 6);
        buffer[3] = 0x80 + ((codepoint & 0x00003F));
        *size = 4;
    }
    else
        return -1;

    return 0;
}
int utf8_check_first(char byte) {
    unsigned char u = (unsigned char)byte;

    if(u < 0x80)
        return 1;

    if(0x80 <= u && u <= 0xBF) {
        /* second, third or fourth byte of a multi-byte
           sequence, i.e. a "continuation byte" */
        return 0;
    }
    else if(u == 0xC0 || u == 0xC1) {
        /* overlong encoding of an ASCII byte */
        return 0;
    }
    else if(0xC2 <= u && u <= 0xDF) {
        /* 2-byte sequence */
        return 2;
    }

    else if(0xE0 <= u && u <= 0xEF) {
        /* 3-byte sequence */
        return 3;
    }
    else if(0xF0 <= u && u <= 0xF4) {
        /* 4-byte sequence */
        return 4;
    }
    else { /* u >= 0xF5 */
        /* Restricted (start of 4-, 5- or 6-byte sequence) or invalid
           UTF-8 */
        return 0;
    }
}
int utf8_check_full(const char *buffer, int size, int32_t *codepoint) {
    int i;
    int32_t value = 0;
    unsigned char u = (unsigned char)buffer[0];

    if(size == 2)
    {
        value = u & 0x1F;
    }
    else if(size == 3)
    {
        value = u & 0xF;
    }
    else if(size == 4)
    {
        value = u & 0x7;
    }
    else
        return 0;

    for(i = 1; i < size; i++)
    {
        u = (unsigned char)buffer[i];

        if(u < 0x80 || u > 0xBF) {
            /* not a continuation byte */
            return 0;
        }

        value = (value << 6) + (u & 0x3F);
    }

    if(value > 0x10FFFF) {
        /* not in Unicode range */
        return 0;
    }

    else if(0xD800 <= value && value <= 0xDFFF) {
        /* invalid code point (UTF-16 surrogate halves) */
        return 0;
    }

    else if((size == 2 && value < 0x80) ||
            (size == 3 && value < 0x800) ||
            (size == 4 && value < 0x10000)) {
        /* overlong encoding */
        return 0;
    }

    if(codepoint)
        *codepoint = value;

    return 1;
}
const char *utf8_iterate(const char *buffer, int32_t *codepoint){
    int count;
    int32_t value;

    if(!*buffer)
        return buffer;

    count = utf8_check_first(buffer[0]);
    if(count <= 0)
        return NULL;

    if(count == 1)
        value = (unsigned char)buffer[0];
    else
    {
        if(!utf8_check_full(buffer, count, &value))
            return NULL;
    }

    if(codepoint)
        *codepoint = value;

    return buffer + count;
}
int utf8_check_string(const char *string, int length){
    int i;

    if(length == -1)
        length = strlen(string);

    for(i = 0; i < length; i++)
    {
        int count = utf8_check_first(string[i]);
        if(count == 0)
            return 0;
        else if(count > 1)
        {
            if(i + count > length)
                return 0;

            if(!utf8_check_full(&string[i], count, NULL))
                return 0;

            i += count - 1;
        }
    }

    return 1;
}

/* utf end */

inline json_t *json_incref(json_t *json) {
  if(json && json->refcount != (unsigned int)-1)
    ++json->refcount;
  return json;
}
inline void json_decref(json_t *json) {
  if(json && json->refcount != (unsigned int)-1 && --json->refcount == 0)
    json_delete(json);
}
/* dump.c */
#define _GNU_SOURCE
#define MAX_INTEGER_STR_LENGTH  100
#define MAX_REAL_STR_LENGTH     100
typedef int (*dump_func)(const char *buffer, int size, void *data);
struct string {
    char *buffer;
    int length;
    int size;
};
static int dump_to_strbuffer(const char *buffer, int size, void *data){
    return strbuffer_append_bytes((strbuffer_t *)data, buffer, size);
}
static int dump_to_file(const char *buffer, int size, void *data){
    FILE *dest = (FILE *)data;
    if(fwrite(buffer, size, 1, dest) != 1)
        return -1;
    return 0;
}
/* 256 spaces (the maximum indentation size) */
static char whitespace[] = "                                                                                                                                                                                                                                                                ";
static int dump_indent(unsigned long flags, int depth, int space, dump_func dump, void *data){
    if(JSON_INDENT(flags) > 0)
    {
        int i, ws_count = JSON_INDENT(flags);

        if(dump("\n", 1, data))
            return -1;

        for(i = 0; i < depth; i++)
        {
            if(dump(whitespace, ws_count, data))
                return -1;
        }
    }
    else if(space && !(flags & JSON_COMPACT))
    {
        return dump(" ", 1, data);
    }
    return 0;
}
static int dump_string(const char *str, int ascii, dump_func dump, void *data){
    const char *pos, *end;
    int32_t codepoint;

    if(dump("\"", 1, data))
        return -1;

    end = pos = str;
    while(1)
    {
        const char *text;
        char seq[13];
        int length;

        while(*end)
        {
            end = utf8_iterate(pos, &codepoint);
            if(!end)
                return -1;

            /* mandatory escape or control char */
            if(codepoint == '\\' || codepoint == '"' || codepoint < 0x20)
                break;

            /* non-ASCII */
            if(ascii && codepoint > 0x7F)
                break;

            pos = end;
        }

        if(pos != str) {
            if(dump(str, pos - str, data))
                return -1;
        }

        if(end == pos)
            break;

        /* handle \, ", and control codes */
        length = 2;
        switch(codepoint)
        {
            case '\\': text = "\\\\"; break;
            case '\"': text = "\\\""; break;
            case '\b': text = "\\b"; break;
            case '\f': text = "\\f"; break;
            case '\n': text = "\\n"; break;
            case '\r': text = "\\r"; break;
            case '\t': text = "\\t"; break;
            default:
            {
                /* codepoint is in BMP */
                if(codepoint < 0x10000)
                {
                    sprintf(seq, "\\u%04x", codepoint);
                    length = 6;
                }

                /* not in BMP -> construct a UTF-16 surrogate pair */
                else
                {
                    int32_t first, last;

                    codepoint -= 0x10000;
                    first = 0xD800 | ((codepoint & 0xffc00) >> 10);
                    last = 0xDC00 | (codepoint & 0x003ff);

                    sprintf(seq, "\\u%04x\\u%04x", first, last);
                    length = 12;
                }

                text = seq;
                break;
            }
        }

        if(dump(text, length, data))
            return -1;

        str = pos = end;
    }

    return dump("\"", 1, data);
}
static int object_key_compare_keys(const void *key1, const void *key2){
    return strcmp((*(const object_key_t **)key1)->key,
                  (*(const object_key_t **)key2)->key);
}
static int object_key_compare_serials(const void *key1, const void *key2){
    return (*(const object_key_t **)key1)->serial -
           (*(const object_key_t **)key2)->serial;
}
static int do_dump(const json_t *json, unsigned long flags, int depth, dump_func dump, void *data) {
  int ascii = flags & JSON_ENSURE_ASCII ? 1 : 0;

  switch(json_typeof(json)) {
      case JSON_NULL:
          return dump("null", 4, data);

      case JSON_TRUE:
          return dump("true", 4, data);

      case JSON_FALSE:
          return dump("false", 5, data);

      case JSON_INTEGER:
      {
          char buffer[MAX_INTEGER_STR_LENGTH];
          int size;

          size = snprintf(buffer, MAX_INTEGER_STR_LENGTH, "%d", json_integer_value(json));
          if(size >= MAX_INTEGER_STR_LENGTH)
              return -1;

          return dump(buffer, size, data);
      }

      case JSON_REAL:
      {
          char buffer[MAX_REAL_STR_LENGTH];
          int size;

          size = snprintf(buffer, MAX_REAL_STR_LENGTH, "%.17g",
                          json_real_value(json));
          if(size >= MAX_REAL_STR_LENGTH)
              return -1;

          /* Make sure there's a dot or 'e' in the output. Otherwise
             a real is converted to an integer when decoding */
          if(strchr(buffer, '.') == NULL &&
             strchr(buffer, 'e') == NULL)
          {
              if(size + 2 >= MAX_REAL_STR_LENGTH) {
                  /* No space to append ".0" */
                  return -1;
              }
              buffer[size] = '.';
              buffer[size + 1] = '0';
              size += 2;
          }

          return dump(buffer, size, data);
      }

      case JSON_STRING:
          return dump_string(json_string_value(json), ascii, dump, data);

      case JSON_ARRAY:
      {
          int i;
          int n;
          json_array_t *array;

          /* detect circular references */
          array = json_to_array(json);
          if(array->visited)
              goto array_error;
          array->visited = 1;

          n = json_array_size(json);

          if(dump("[", 1, data))
              goto array_error;
          if(n == 0) {
              array->visited = 0;
              return dump("]", 1, data);
          }
          if(dump_indent(flags, depth + 1, 0, dump, data))
              goto array_error;

          for(i = 0; i < n; ++i) {
              if(do_dump(json_array_get(json, i), flags, depth + 1,
                         dump, data))
                  goto array_error;

              if(i < n - 1)
              {
                  if(dump(",", 1, data) ||
                     dump_indent(flags, depth + 1, 1, dump, data))
                      goto array_error;
              }
              else
              {
                  if(dump_indent(flags, depth, 0, dump, data))
                      goto array_error;
              }
          }

          array->visited = 0;
          return dump("]", 1, data);

      array_error:
          array->visited = 0;
          return -1;
      }

      case JSON_OBJECT:
      {
          json_object_t *object;
          void *iter;
          const char *separator;
          int separator_length;

          if(flags & JSON_COMPACT) {
              separator = ":";
              separator_length = 1;
          }
          else {
              separator = ": ";
              separator_length = 2;
          }

          /* detect circular references */
          object = json_to_object(json);
          if(object->visited)
              goto object_error;
          object->visited = 1;

          iter = json_object_iter((json_t *)json);

          if(dump("{", 1, data))
              goto object_error;
          if(!iter) {
              object->visited = 0;
              return dump("}", 1, data);
          }
          if(dump_indent(flags, depth + 1, 0, dump, data))
              goto object_error;

          if(flags & JSON_SORT_KEYS || flags & JSON_PRESERVE_ORDER)
          {
              const object_key_t **keys;
              unsigned int size;
              unsigned int i;
              int (*cmp_func)(const void *, const void *);

              size = json_object_size(json);
              keys = malloc(size * sizeof(object_key_t *));
              if(!keys)
                  goto object_error;

              i = 0;
              while(iter)
              {
                  keys[i] = jsonp_object_iter_fullkey(iter);
                  iter = json_object_iter_next((json_t *)json, iter);
                  i++;
              }
              assert(i == size);

              if(flags & JSON_SORT_KEYS)
                  cmp_func = object_key_compare_keys;
              else
                  cmp_func = object_key_compare_serials;

              qsort(keys, size, sizeof(object_key_t *), cmp_func);

              for(i = 0; i < size; i++)
              {
                  const char *key;
                  json_t *value;

                  key = keys[i]->key;
                  value = json_object_get(json, key);
                  assert(value);

                  dump_string(key, ascii, dump, data);
                  if(dump(separator, separator_length, data) ||
                     do_dump(value, flags, depth + 1, dump, data))
                  {
                      free(keys);
                      goto object_error;
                  }

                  if(i < size - 1)
                  {
                      if(dump(",", 1, data) ||
                         dump_indent(flags, depth + 1, 1, dump, data))
                      {
                          free(keys);
                          goto object_error;
                      }
                  }
                  else
                  {
                      if(dump_indent(flags, depth, 0, dump, data))
                      {
                          free(keys);
                          goto object_error;
                      }
                  }
              }

              free(keys);
          }
          else
          {
              /* Don't sort keys */

              while(iter)
              {
                  void *next = json_object_iter_next((json_t *)json, iter);

                  dump_string(json_object_iter_key(iter), ascii, dump, data);
                  if(dump(separator, separator_length, data) ||
                     do_dump(json_object_iter_value(iter), flags, depth + 1,
                             dump, data))
                      goto object_error;

                  if(next)
                  {
                      if(dump(",", 1, data) ||
                         dump_indent(flags, depth + 1, 1, dump, data))
                          goto object_error;
                  }
                  else
                  {
                      if(dump_indent(flags, depth, 0, dump, data))
                          goto object_error;
                  }

                  iter = next;
              }
          }

          object->visited = 0;
          return dump("}", 1, data);

      object_error:
          object->visited = 0;
          return -1;
      }

      default:
          /* not reached */
          return -1;
  }
}

char *json_dumps(const json_t *json, unsigned long flags) {
  strbuffer_t strbuff;
  char *result;
  if((!json_is_array(json) && !json_is_object(json)) || (strbuffer_init(&strbuff)))
    return NULL;
  if(do_dump(json, flags, 0, dump_to_strbuffer, (void *)&strbuff)) {
    strbuffer_close(&strbuff);
    return NULL;
  }
  result = strdup(strbuffer_value(&strbuff));
  strbuffer_close(&strbuff);
  return result;
}
int json_dumpf(const json_t *json, FILE *output, unsigned long flags) {
  if(!json_is_array(json) && !json_is_object(json))
    return -1;
  return do_dump(json, flags, 0, dump_to_file, (void *)output);
}
int json_dump_file(const json_t *json, const char *path, unsigned long flags) {
  int result;
  FILE *output = fopen(path, "w");
  if(!output)
    return -1;
  result = json_dumpf(json, output, flags);
  fclose(output);
  return result;
}
/* dump.c end */

/* value */

static inline void json_init(json_t *json, json_type type)
{
    json->type = type;
    json->refcount = 1;
}


/*** object ***/

#define string_to_key(string)  container_of(string, object_key_t, key)

static unsigned int hash_key(const void *ptr)
{
    const char *str = ((const object_key_t *)ptr)->key;

    unsigned int hash = 5381;
    unsigned int c;

    while((c = (unsigned int)*str))
    {
        hash = ((hash << 5) + hash) + c;
        str++;
    }

    return hash;
}

static int key_equal(const void *ptr1, const void *ptr2)
{
    return strcmp(((const object_key_t *)ptr1)->key,
                  ((const object_key_t *)ptr2)->key) == 0;
}

static void value_decref(void *value)
{
    json_decref((json_t *)value);
}

json_t *json_object(void)
{
    json_object_t *object = malloc(sizeof(json_object_t));
    if(!object)
        return NULL;
    json_init(&object->json, JSON_OBJECT);

    if(hashtable_init(&object->hashtable, hash_key, key_equal,
                      free, value_decref))
    {
        free(object);
        return NULL;
    }

    object->serial = 0;
    object->visited = 0;

    return &object->json;
}

static void json_delete_object(json_object_t *object)
{
    hashtable_close(&object->hashtable);
    free(object);
}

unsigned int json_object_size(const json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return -1;

    object = json_to_object(json);
    return object->hashtable.size;
}

json_t *json_object_get(const json_t *json, const char *key)
{
    json_object_t *object;

    if(!json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_get(&object->hashtable, string_to_key(key));
}

int json_object_set_new_nocheck(json_t *json, const char *key, json_t *value)
{
    json_object_t *object;
    object_key_t *k;

    if(!key || !value)
        return -1;

    if(!json_is_object(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    object = json_to_object(json);

    k = malloc(sizeof(object_key_t) + strlen(key) + 1);
    if(!k)
        return -1;

    k->serial = object->serial++;
    strcpy(k->key, key);

    if(hashtable_set(&object->hashtable, k, value))
    {
        json_decref(value);
        return -1;
    }

    return 0;
}

int json_object_set_new(json_t *json, const char *key, json_t *value)
{
    if(!key || !utf8_check_string(key, -1))
    {
        json_decref(value);
        return -1;
    }

    return json_object_set_new_nocheck(json, key, value);
}

int json_object_del(json_t *json, const char *key)
{
    json_object_t *object;

    if(!json_is_object(json))
        return -1;

    object = json_to_object(json);
    return hashtable_del(&object->hashtable, string_to_key(key));
}

int json_object_clear(json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return -1;

    object = json_to_object(json);
    hashtable_clear(&object->hashtable);

    return 0;
}

int json_object_update(json_t *object, json_t *other)
{
    void *iter;

    if(!json_is_object(object) || !json_is_object(other))
        return -1;

    iter = json_object_iter(other);
    while(iter) {
        const char *key;
        json_t *value;

        key = json_object_iter_key(iter);
        value = json_object_iter_value(iter);

        if(json_object_set_nocheck(object, key, value))
            return -1;

        iter = json_object_iter_next(other, iter);
    }

    return 0;
}

void *json_object_iter(json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_iter(&object->hashtable);
}

void *json_object_iter_at(json_t *json, const char *key)
{
    json_object_t *object;

    if(!key || !json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_iter_at(&object->hashtable, string_to_key(key));
}

void *json_object_iter_next(json_t *json, void *iter)
{
    json_object_t *object;

    if(!json_is_object(json) || iter == NULL)
        return NULL;

    object = json_to_object(json);
    return hashtable_iter_next(&object->hashtable, iter);
}

const object_key_t *jsonp_object_iter_fullkey(void *iter)
{
    if(!iter)
        return NULL;

    return hashtable_iter_key(iter);
}

const char *json_object_iter_key(void *iter)
{
    if(!iter)
        return NULL;

    return jsonp_object_iter_fullkey(iter)->key;
}

json_t *json_object_iter_value(void *iter)
{
    if(!iter)
        return NULL;

    return (json_t *)hashtable_iter_value(iter);
}

int json_object_iter_set_new(json_t *json, void *iter, json_t *value)
{
    json_object_t *object;

    if(!json_is_object(json) || !iter || !value)
        return -1;

    object = json_to_object(json);
    hashtable_iter_set(&object->hashtable, iter, value);

    return 0;
}

static int json_object_equal(json_t *object1, json_t *object2)
{
    void *iter;

    if(json_object_size(object1) != json_object_size(object2))
        return 0;

    iter = json_object_iter(object1);
    while(iter)
    {
        const char *key;
        json_t *value1, *value2;

        key = json_object_iter_key(iter);
        value1 = json_object_iter_value(iter);
        value2 = json_object_get(object2, key);

        if(!json_equal(value1, value2))
            return 0;

        iter = json_object_iter_next(object1, iter);
    }

    return 1;
}

static json_t *json_object_copy(json_t *object)
{
    json_t *result;
    void *iter;

    result = json_object();
    if(!result)
        return NULL;

    iter = json_object_iter(object);
    while(iter)
    {
        const char *key;
        json_t *value;

        key = json_object_iter_key(iter);
        value = json_object_iter_value(iter);
        json_object_set_nocheck(result, key, value);

        iter = json_object_iter_next(object, iter);
    }

    return result;
}

static json_t *json_object_deep_copy(json_t *object)
{
    json_t *result;
    void *iter;

    result = json_object();
    if(!result)
        return NULL;

    iter = json_object_iter(object);
    while(iter)
    {
        const char *key;
        json_t *value;

        key = json_object_iter_key(iter);
        value = json_object_iter_value(iter);
        json_object_set_new_nocheck(result, key, json_deep_copy(value));

        iter = json_object_iter_next(object, iter);
    }

    return result;
}


/*** array ***/

json_t *json_array(void)
{
    json_array_t *array = malloc(sizeof(json_array_t));
    if(!array)
        return NULL;
    json_init(&array->json, JSON_ARRAY);

    array->entries = 0;
    array->size = 8;

    array->table = malloc(array->size * sizeof(json_t *));
    if(!array->table) {
        free(array);
        return NULL;
    }

    array->visited = 0;

    return &array->json;
}

static void json_delete_array(json_array_t *array) {
    unsigned int i;

    for(i = 0; i < array->entries; i++)
        json_decref(array->table[i]);

    free(array->table);
    free(array);
}
inline int json_object_set(json_t *object, const char *key, json_t *value) {
    return json_object_set_new(object, key, json_incref(value));
}
inline int json_object_set_nocheck(json_t *object, const char *key, json_t *value) {
    return json_object_set_new_nocheck(object, key, json_incref(value));
}
inline int json_object_iter_set(json_t *object, void *iter, json_t *value) {
    return json_object_iter_set_new(object, iter, json_incref(value));
}
unsigned int json_array_size(const json_t *json) {
    if(!json_is_array(json))
        return 0;

    return json_to_array(json)->entries;
}

json_t *json_array_get(const json_t *json, unsigned int index)
{
    json_array_t *array;
    if(!json_is_array(json))
        return NULL;
    array = json_to_array(json);

    if(index >= array->entries)
        return NULL;

    return array->table[index];
}

int json_array_set_new(json_t *json, unsigned int index, json_t *value)
{
    json_array_t *array;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(index >= array->entries)
    {
        json_decref(value);
        return -1;
    }

    json_decref(array->table[index]);
    array->table[index] = value;

    return 0;
}

static void array_move(json_array_t *array, unsigned int dest,
                       unsigned int src, unsigned int count)
{
    memmove(&array->table[dest], &array->table[src], count * sizeof(json_t *));
}

static void array_copy(json_t **dest, unsigned int dpos,
                       json_t **src, unsigned int spos,
                       unsigned int count)
{
    memcpy(&dest[dpos], &src[spos], count * sizeof(json_t *));
}

static json_t **json_array_grow(json_array_t *array,
                                unsigned int amount,
                                int copy)
{
    unsigned int new_size;
    json_t **old_table, **new_table;

    if(array->entries + amount <= array->size)
        return array->table;

    old_table = array->table;

    new_size = max(array->size + amount, array->size * 2);
    new_table = malloc(new_size * sizeof(json_t *));
    if(!new_table)
        return NULL;

    array->size = new_size;
    array->table = new_table;

    if(copy) {
        array_copy(array->table, 0, old_table, 0, array->entries);
        free(old_table);
        return array->table;
    }

    return old_table;
}

int json_array_append_new(json_t *json, json_t *value)
{
    json_array_t *array;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(!json_array_grow(array, 1, 1)) {
        json_decref(value);
        return -1;
    }

    array->table[array->entries] = value;
    array->entries++;

    return 0;
}

int json_array_insert_new(json_t *json, unsigned int index, json_t *value)
{
    json_array_t *array;
    json_t **old_table;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value) {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(index > array->entries) {
        json_decref(value);
        return -1;
    }

    old_table = json_array_grow(array, 1, 0);
    if(!old_table) {
        json_decref(value);
        return -1;
    }

    if(old_table != array->table) {
        array_copy(array->table, 0, old_table, 0, index);
        array_copy(array->table, index + 1, old_table, index,
                   array->entries - index);
        free(old_table);
    }
    else
        array_move(array, index + 1, index, array->entries - index);

    array->table[index] = value;
    array->entries++;

    return 0;
}

int json_array_remove(json_t *json, unsigned int index)
{
    json_array_t *array;

    if(!json_is_array(json))
        return -1;
    array = json_to_array(json);

    if(index >= array->entries)
        return -1;

    json_decref(array->table[index]);

    array_move(array, index, index + 1, array->entries - index);
    array->entries--;

    return 0;
}

int json_array_clear(json_t *json)
{
    json_array_t *array;
    unsigned int i;

    if(!json_is_array(json))
        return -1;
    array = json_to_array(json);

    for(i = 0; i < array->entries; i++)
        json_decref(array->table[i]);

    array->entries = 0;
    return 0;
}

int json_array_extend(json_t *json, json_t *other_json)
{
    json_array_t *array, *other;
    unsigned int i;

    if(!json_is_array(json) || !json_is_array(other_json))
        return -1;
    array = json_to_array(json);
    other = json_to_array(other_json);

    if(!json_array_grow(array, other->entries, 1))
        return -1;

    for(i = 0; i < other->entries; i++)
        json_incref(other->table[i]);

    array_copy(array->table, array->entries, other->table, 0, other->entries);

    array->entries += other->entries;
    return 0;
}

static int json_array_equal(json_t *array1, json_t *array2)
{
    unsigned int i, size;

    size = json_array_size(array1);
    if(size != json_array_size(array2))
        return 0;

    for(i = 0; i < size; i++)
    {
        json_t *value1, *value2;

        value1 = json_array_get(array1, i);
        value2 = json_array_get(array2, i);

        if(!json_equal(value1, value2))
            return 0;
    }

    return 1;
}

static json_t *json_array_copy(json_t *array)
{
    json_t *result;
    unsigned int i;

    result = json_array();
    if(!result)
        return NULL;

    for(i = 0; i < json_array_size(array); i++)
        json_array_append(result, json_array_get(array, i));

    return result;
}

static json_t *json_array_deep_copy(json_t *array)
{
    json_t *result;
    unsigned int i;

    result = json_array();
    if(!result)
        return NULL;

    for(i = 0; i < json_array_size(array); i++)
        json_array_append_new(result, json_deep_copy(json_array_get(array, i)));

    return result;
}

/*** string ***/

json_t *json_string_nocheck(const char *value)
{
    json_string_t *string;

    if(!value)
        return NULL;

    string = malloc(sizeof(json_string_t));
    if(!string)
        return NULL;
    json_init(&string->json, JSON_STRING);

    string->value = strdup(value);
    if(!string->value) {
        free(string);
        return NULL;
    }

    return &string->json;
}

json_t *json_string(const char *value)
{
    if(!value || !utf8_check_string(value, -1))
        return NULL;

    return json_string_nocheck(value);
}
inline int json_array_set(json_t *array, unsigned int index, json_t *value) {
    return json_array_set_new(array, index, json_incref(value));
}
inline int json_array_append(json_t *array, json_t *value) {
    return json_array_append_new(array, json_incref(value));
}
inline int json_array_insert(json_t *array, unsigned int index, json_t *value) {
    return json_array_insert_new(array, index, json_incref(value));
}
const char *json_string_value(const json_t *json)
{
    if(!json_is_string(json))
        return NULL;

    return json_to_string(json)->value;
}

int json_string_set_nocheck(json_t *json, const char *value)
{
    char *dup;
    json_string_t *string;

    dup = strdup(value);
    if(!dup)
        return -1;

    string = json_to_string(json);
    free(string->value);
    string->value = dup;

    return 0;
}

int json_string_set(json_t *json, const char *value)
{
    if(!value || !utf8_check_string(value, -1))
        return -1;

    return json_string_set_nocheck(json, value);
}

static void json_delete_string(json_string_t *string)
{
    free(string->value);
    free(string);
}

static int json_string_equal(json_t *string1, json_t *string2)
{
    return strcmp(json_string_value(string1), json_string_value(string2)) == 0;
}

static json_t *json_string_copy(json_t *string)
{
    return json_string_nocheck(json_string_value(string));
}


/*** integer ***/

json_t *json_integer(int value)
{
    json_integer_t *integer = malloc(sizeof(json_integer_t));
    if(!integer)
        return NULL;
    json_init(&integer->json, JSON_INTEGER);

    integer->value = value;
    return &integer->json;
}

int json_integer_value(const json_t *json)
{
    if(!json_is_integer(json))
        return 0;

    return json_to_integer(json)->value;
}

int json_integer_set(json_t *json, int value)
{
    if(!json_is_integer(json))
        return -1;

    json_to_integer(json)->value = value;

    return 0;
}

static void json_delete_integer(json_integer_t *integer)
{
    free(integer);
}

static int json_integer_equal(json_t *integer1, json_t *integer2)
{
    return json_integer_value(integer1) == json_integer_value(integer2);
}

static json_t *json_integer_copy(json_t *integer)
{
    return json_integer(json_integer_value(integer));
}


/*** real ***/

json_t *json_real(double value)
{
    json_real_t *real = malloc(sizeof(json_real_t));
    if(!real)
        return NULL;
    json_init(&real->json, JSON_REAL);

    real->value = value;
    return &real->json;
}

double json_real_value(const json_t *json)
{
    if(!json_is_real(json))
        return 0;

    return json_to_real(json)->value;
}

int json_real_set(json_t *json, double value)
{
    if(!json_is_real(json))
        return 0;

    json_to_real(json)->value = value;

    return 0;
}

static void json_delete_real(json_real_t *real)
{
    free(real);
}

static int json_real_equal(json_t *real1, json_t *real2)
{
    return json_real_value(real1) == json_real_value(real2);
}

static json_t *json_real_copy(json_t *real)
{
    return json_real(json_real_value(real));
}


/*** number ***/

double json_number_value(const json_t *json)
{
    if(json_is_integer(json))
        return json_integer_value(json);
    else if(json_is_real(json))
        return json_real_value(json);
    else
        return 0.0;
}


/*** simple values ***/

json_t *json_true(void)
{
    static json_t the_true = {
        .type = JSON_TRUE,
        .refcount = (unsigned int)-1
    };
    return &the_true;
}


json_t *json_false(void)
{
    static json_t the_false = {
        .type = JSON_FALSE,
        .refcount = (unsigned int)-1
    };
    return &the_false;
}


json_t *json_null(void)
{
    static json_t the_null = {
        .type = JSON_NULL,
        .refcount = (unsigned int)-1
    };
    return &the_null;
}


/*** deletion ***/

void json_delete(json_t *json)
{
    if(json_is_object(json))
        json_delete_object(json_to_object(json));

    else if(json_is_array(json))
        json_delete_array(json_to_array(json));

    else if(json_is_string(json))
        json_delete_string(json_to_string(json));

    else if(json_is_integer(json))
        json_delete_integer(json_to_integer(json));

    else if(json_is_real(json))
        json_delete_real(json_to_real(json));

    /* json_delete is not called for true, false or null */
}


/*** equality ***/

int json_equal(json_t *json1, json_t *json2)
{
    if(!json1 || !json2)
        return 0;

    if(json_typeof(json1) != json_typeof(json2))
        return 0;

    /* this covers true, false and null as they are singletons */
    if(json1 == json2)
        return 1;

    if(json_is_object(json1))
        return json_object_equal(json1, json2);

    if(json_is_array(json1))
        return json_array_equal(json1, json2);

    if(json_is_string(json1))
        return json_string_equal(json1, json2);

    if(json_is_integer(json1))
        return json_integer_equal(json1, json2);

    if(json_is_real(json1))
        return json_real_equal(json1, json2);

    return 0;
}


/*** copying ***/

json_t *json_copy(json_t *json)
{
    if(!json)
        return NULL;

    if(json_is_object(json))
        return json_object_copy(json);

    if(json_is_array(json))
        return json_array_copy(json);

    if(json_is_string(json))
        return json_string_copy(json);

    if(json_is_integer(json))
        return json_integer_copy(json);

    if(json_is_real(json))
        return json_real_copy(json);

    if(json_is_true(json) || json_is_false(json) || json_is_null(json))
        return json;

    return NULL;
}

json_t *json_deep_copy(json_t *json)
{
    if(!json)
        return NULL;

    if(json_is_object(json))
        return json_object_deep_copy(json);

    if(json_is_array(json))
        return json_array_deep_copy(json);

    /* for the rest of the types, deep copying doesn't differ from
       shallow copying */

    if(json_is_string(json))
        return json_string_copy(json);

    if(json_is_integer(json))
        return json_integer_copy(json);

    if(json_is_real(json))
        return json_real_copy(json);

    if(json_is_true(json) || json_is_false(json) || json_is_null(json))
        return json;

    return NULL;
}

/* value end */
/* load */
#define TOKEN_INVALID         -1
#define TOKEN_EOF              0
#define TOKEN_STRING         256
#define TOKEN_INTEGER        257
#define TOKEN_REAL           258
#define TOKEN_TRUE           259
#define TOKEN_FALSE          260
#define TOKEN_NULL           261

/* read one byte from stream, return EOF on end of file */
typedef int (*get_func)(void *data);

/* return non-zero if end of file has been reached */
typedef int (*eof_func)(void *data);

struct stream_t {
    get_func get;
    eof_func eof;
    void *data;
    int stream_pos;
    char buffer[5];
    int buffer_pos;
};
struct lex_t {
    stream_t stream;
    strbuffer_t saved_text;
    int token;
    int line, column;
    union {
        char *string;
        int integer;
        double real;
    } value;
};


/*** error reporting ***/

static void error_init(json_error_t *error)
{
    if(error)
    {
        error->text[0] = '\0';
        error->line = -1;
    }
}

static void error_set(json_error_t *error, const lex_t *lex,
                      const char *msg, ...)
{
    va_list ap;
    char text[JSON_ERROR_TEXT_LENGTH];

    if(!error || error->text[0] != '\0') {
        /* error already set */
        return;
    }

    va_start(ap, msg);
    vsnprintf(text, JSON_ERROR_TEXT_LENGTH, msg, ap);
    va_end(ap);

    if(lex)
    {
        const char *saved_text = strbuffer_value(&lex->saved_text);
        error->line = lex->line;
        if(saved_text && saved_text[0])
        {
            if(lex->saved_text.length <= 20) {
                snprintf(error->text, JSON_ERROR_TEXT_LENGTH,
                         "%s near '%s'", text, saved_text);
            }
            else
                snprintf(error->text, JSON_ERROR_TEXT_LENGTH, "%s", text);
        }
        else
        {
            snprintf(error->text, JSON_ERROR_TEXT_LENGTH,
                     "%s near end of file", text);
        }
    }
    else
    {
        error->line = -1;
        snprintf(error->text, JSON_ERROR_TEXT_LENGTH, "%s", text);
    }
}


/*** lexical analyzer ***/

static void
stream_init(stream_t *stream, get_func get, eof_func eof, void *data)
{
    stream->get = get;
    stream->eof = eof;
    stream->data = data;
    stream->stream_pos = 0;
    stream->buffer[0] = '\0';
    stream->buffer_pos = 0;
}

static char stream_get(stream_t *stream, json_error_t *error)
{
    char c;

    if(!stream->buffer[stream->buffer_pos])
    {
        stream->buffer[0] = stream->get(stream->data);
        stream->buffer_pos = 0;

        c = stream->buffer[0];

        if((unsigned char)c >= 0x80 && c != (char)EOF)
        {
            /* multi-byte UTF-8 sequence */
            int i, count;

            count = utf8_check_first(c);
            if(!count)
                goto out;

            assert(count >= 2);

            for(i = 1; i < count; i++)
                stream->buffer[i] = stream->get(stream->data);

            if(!utf8_check_full(stream->buffer, count, NULL))
                goto out;

            stream->stream_pos += count;
            stream->buffer[count] = '\0';
        }
        else {
            stream->buffer[1] = '\0';
            stream->stream_pos++;
        }
    }

    return stream->buffer[stream->buffer_pos++];

out:
    error_set(error, NULL, "unable to decode byte 0x%x at position %d",
              (unsigned char)c, stream->stream_pos);

    stream->buffer[0] = EOF;
    stream->buffer[1] = '\0';
    stream->buffer_pos = 1;

    return EOF;
}

static void stream_unget(stream_t *stream, char c)
{
    assert(stream->buffer_pos > 0);
    stream->buffer_pos--;
    assert(stream->buffer[stream->buffer_pos] == c);
}


static int lex_get(lex_t *lex, json_error_t *error)
{
    return stream_get(&lex->stream, error);
}

static int lex_eof(lex_t *lex)
{
    return lex->stream.eof(lex->stream.data);
}

static void lex_save(lex_t *lex, char c)
{
    strbuffer_append_byte(&lex->saved_text, c);
}

static int lex_get_save(lex_t *lex, json_error_t *error)
{
    char c = stream_get(&lex->stream, error);
    lex_save(lex, c);
    return c;
}

static void lex_unget_unsave(lex_t *lex, char c)
{
    char d;
    stream_unget(&lex->stream, c);
    d = strbuffer_pop(&lex->saved_text);
    assert(c == d);
}

static void lex_save_cached(lex_t *lex)
{
    while(lex->stream.buffer[lex->stream.buffer_pos] != '\0')
    {
        lex_save(lex, lex->stream.buffer[lex->stream.buffer_pos]);
        lex->stream.buffer_pos++;
    }
}

/* assumes that str points to 'u' plus at least 4 valid hex digits */
static int32_t decode_unicode_escape(const char *str)
{
    int i;
    int32_t value = 0;

    assert(str[0] == 'u');

    for(i = 1; i <= 4; i++) {
        char c = str[i];
        value <<= 4;
        if(isdigit(c))
            value += c - '0';
        else if(islower(c))
            value += c - 'a' + 10;
        else if(isupper(c))
            value += c - 'A' + 10;
        else
            assert(0);
    }

    return value;
}

static void lex_scan_string(lex_t *lex, json_error_t *error)
{
    char c;
    const char *p;
    char *t;
    int i;

    lex->value.string = NULL;
    lex->token = TOKEN_INVALID;

    c = lex_get_save(lex, error);

    while(c != '"') {
        if(c == (char)EOF) {
            lex_unget_unsave(lex, c);
            if(lex_eof(lex))
                error_set(error, lex, "premature end of input");
            goto out;
        }

        else if((unsigned char)c <= 0x1F) {
            /* control character */
            lex_unget_unsave(lex, c);
            if(c == '\n')
                error_set(error, lex, "unexpected newline", c);
            else
                error_set(error, lex, "control character 0x%x", c);
            goto out;
        }

        else if(c == '\\') {
            c = lex_get_save(lex, error);
            if(c == 'u') {
                c = lex_get_save(lex, error);
                for(i = 0; i < 4; i++) {
                    if(!isxdigit(c)) {
                        lex_unget_unsave(lex, c);
                        error_set(error, lex, "invalid escape");
                        goto out;
                    }
                    c = lex_get_save(lex, error);
                }
            }
            else if(c == '"' || c == '\\' || c == '/' || c == 'b' ||
                    c == 'f' || c == 'n' || c == 'r' || c == 't')
                c = lex_get_save(lex, error);
            else {
                lex_unget_unsave(lex, c);
                error_set(error, lex, "invalid escape");
                goto out;
            }
        }
        else
            c = lex_get_save(lex, error);
    }

    /* the actual value is at most of the same length as the source
       string, because:
         - shortcut escapes (e.g. "\t") (length 2) are converted to 1 byte
         - a single \uXXXX escape (length 6) is converted to at most 3 bytes
         - two \uXXXX escapes (length 12) forming an UTF-16 surrogate pair
           are converted to 4 bytes
    */
    lex->value.string = malloc(lex->saved_text.length + 1);
    if(!lex->value.string) {
        /* this is not very nice, since TOKEN_INVALID is returned */
        goto out;
    }

    /* the target */
    t = lex->value.string;

    /* + 1 to skip the " */
    p = strbuffer_value(&lex->saved_text) + 1;

    while(*p != '"') {
        if(*p == '\\') {
            p++;
            if(*p == 'u') {
                char buffer[4];
                int length;
                int32_t value;

                value = decode_unicode_escape(p);
                p += 5;

                if(0xD800 <= value && value <= 0xDBFF) {
                    /* surrogate pair */
                    if(*p == '\\' && *(p + 1) == 'u') {
                        int32_t value2 = decode_unicode_escape(++p);
                        p += 5;

                        if(0xDC00 <= value2 && value2 <= 0xDFFF) {
                            /* valid second surrogate */
                            value =
                                ((value - 0xD800) << 10) +
                                (value2 - 0xDC00) +
                                0x10000;
                        }
                        else {
                            /* invalid second surrogate */
                            error_set(error, lex,
                                      "invalid Unicode '\\u%04X\\u%04X'",
                                      value, value2);
                            goto out;
                        }
                    }
                    else {
                        /* no second surrogate */
                        error_set(error, lex, "invalid Unicode '\\u%04X'",
                                  value);
                        goto out;
                    }
                }
                else if(0xDC00 <= value && value <= 0xDFFF) {
                    error_set(error, lex, "invalid Unicode '\\u%04X'", value);
                    goto out;
                }
                else if(value == 0)
                {
                    error_set(error, lex, "\\u0000 is not allowed");
                    goto out;
                }

                if(utf8_encode(value, buffer, &length))
                    assert(0);

                memcpy(t, buffer, length);
                t += length;
            }
            else {
                switch(*p) {
                    case '"': case '\\': case '/':
                        *t = *p; break;
                    case 'b': *t = '\b'; break;
                    case 'f': *t = '\f'; break;
                    case 'n': *t = '\n'; break;
                    case 'r': *t = '\r'; break;
                    case 't': *t = '\t'; break;
                    default: assert(0);
                }
                t++;
                p++;
            }
        }
        else
            *(t++) = *(p++);
    }
    *t = '\0';
    lex->token = TOKEN_STRING;
    return;

out:
    free(lex->value.string);
}

static int lex_scan_number(lex_t *lex, char c, json_error_t *error)
{
    const char *saved_text;
    char *end;
    double value;

    lex->token = TOKEN_INVALID;

    if(c == '-')
        c = lex_get_save(lex, error);

    if(c == '0') {
        c = lex_get_save(lex, error);
        if(isdigit(c)) {
            lex_unget_unsave(lex, c);
            goto out;
        }
    }
    else if(isdigit(c)) {
        c = lex_get_save(lex, error);
        while(isdigit(c))
            c = lex_get_save(lex, error);
    }
    else {
      lex_unget_unsave(lex, c);
      goto out;
    }

    if(c != '.' && c != 'E' && c != 'e') {
        long value;

        lex_unget_unsave(lex, c);

        saved_text = strbuffer_value(&lex->saved_text);
        value = strtol(saved_text, &end, 10);
        assert(end == saved_text + lex->saved_text.length);

        if((value == LONG_MAX && errno == ERANGE) || value > INT_MAX) {
            error_set(error, lex, "too big integer");
            goto out;
        }
        else if((value == LONG_MIN && errno == ERANGE) || value < INT_MIN) {
            error_set(error, lex, "too big negative integer");
            goto out;
        }

        lex->token = TOKEN_INTEGER;
        lex->value.integer = (int)value;
        return 0;
    }

    if(c == '.') {
        c = lex_get(lex, error);
        if(!isdigit(c))
            goto out;
        lex_save(lex, c);

        c = lex_get_save(lex, error);
        while(isdigit(c))
            c = lex_get_save(lex, error);
    }

    if(c == 'E' || c == 'e') {
        c = lex_get_save(lex, error);
        if(c == '+' || c == '-')
            c = lex_get_save(lex, error);

        if(!isdigit(c)) {
            lex_unget_unsave(lex, c);
            goto out;
        }

        c = lex_get_save(lex, error);
        while(isdigit(c))
            c = lex_get_save(lex, error);
    }

    lex_unget_unsave(lex, c);

    saved_text = strbuffer_value(&lex->saved_text);
    value = strtod(saved_text, &end);
    assert(end == saved_text + lex->saved_text.length);

    if(errno == ERANGE && value != 0) {
        error_set(error, lex, "real number overflow");
        goto out;
    }

    lex->token = TOKEN_REAL;
    lex->value.real = value;
    return 0;

out:
    return -1;
}

static int lex_scan(lex_t *lex, json_error_t *error)
{
    char c;

    strbuffer_clear(&lex->saved_text);

    if(lex->token == TOKEN_STRING) {
        free(lex->value.string);
        lex->value.string = NULL;
    }

    c = lex_get(lex, error);
    while(c == ' ' || c == '\t' || c == '\n' || c == '\r')
    {
        if(c == '\n')
            lex->line++;

        c = lex_get(lex, error);
    }

    if(c == (char)EOF) {
        if(lex_eof(lex))
            lex->token = TOKEN_EOF;
        else
            lex->token = TOKEN_INVALID;
        goto out;
    }

    lex_save(lex, c);

    if(c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',')
        lex->token = c;

    else if(c == '"')
        lex_scan_string(lex, error);

    else if(isdigit(c) || c == '-') {
        if(lex_scan_number(lex, c, error))
            goto out;
    }

    else if(isupper(c) || islower(c)) {
        /* eat up the whole identifier for clearer error messages */
        const char *saved_text;

        c = lex_get_save(lex, error);
        while(isupper(c) || islower(c))
            c = lex_get_save(lex, error);
        lex_unget_unsave(lex, c);

        saved_text = strbuffer_value(&lex->saved_text);

        if(strcmp(saved_text, "true") == 0)
            lex->token = TOKEN_TRUE;
        else if(strcmp(saved_text, "false") == 0)
            lex->token = TOKEN_FALSE;
        else if(strcmp(saved_text, "null") == 0)
            lex->token = TOKEN_NULL;
        else
            lex->token = TOKEN_INVALID;
    }

    else {
        /* save the rest of the input UTF-8 sequence to get an error
           message of valid UTF-8 */
        lex_save_cached(lex);
        lex->token = TOKEN_INVALID;
    }

out:
    return lex->token;
}

static char *lex_steal_string(lex_t *lex)
{
    char *result = NULL;
    if(lex->token == TOKEN_STRING)
    {
        result = lex->value.string;
        lex->value.string = NULL;
    }
    return result;
}

static int lex_init(lex_t *lex, get_func get, eof_func eof, void *data)
{
    stream_init(&lex->stream, get, eof, data);
    if(strbuffer_init(&lex->saved_text))
        return -1;

    lex->token = TOKEN_INVALID;
    lex->line = 1;

    return 0;
}

static void lex_close(lex_t *lex)
{
    if(lex->token == TOKEN_STRING)
        free(lex->value.string);
    strbuffer_close(&lex->saved_text);
}


/*** parser ***/

static json_t *parse_value(lex_t *lex, json_error_t *error);

static json_t *parse_object(lex_t *lex, json_error_t *error)
{
    json_t *object = json_object();
    if(!object)
        return NULL;

    lex_scan(lex, error);
    if(lex->token == '}')
        return object;

    while(1) {
        char *key;
        json_t *value;

        if(lex->token != TOKEN_STRING) {
            error_set(error, lex, "string or '}' expected");
            goto error;
        }

        key = lex_steal_string(lex);
        if(!key)
            return NULL;

        lex_scan(lex, error);
        if(lex->token != ':') {
            free(key);
            error_set(error, lex, "':' expected");
            goto error;
        }

        lex_scan(lex, error);
        value = parse_value(lex, error);
        if(!value) {
            free(key);
            goto error;
        }

        if(json_object_set_nocheck(object, key, value)) {
            free(key);
            json_decref(value);
            goto error;
        }

        json_decref(value);
        free(key);

        lex_scan(lex, error);
        if(lex->token != ',')
            break;

        lex_scan(lex, error);
    }

    if(lex->token != '}') {
        error_set(error, lex, "'}' expected");
        goto error;
    }

    return object;

error:
    json_decref(object);
    return NULL;
}

static json_t *parse_array(lex_t *lex, json_error_t *error)
{
    json_t *array = json_array();
    if(!array)
        return NULL;

    lex_scan(lex, error);
    if(lex->token == ']')
        return array;

    while(lex->token) {
        json_t *elem = parse_value(lex, error);
        if(!elem)
            goto error;

        if(json_array_append(array, elem)) {
            json_decref(elem);
            goto error;
        }
        json_decref(elem);

        lex_scan(lex, error);
        if(lex->token != ',')
            break;

        lex_scan(lex, error);
    }

    if(lex->token != ']') {
        error_set(error, lex, "']' expected");
        goto error;
    }

    return array;

error:
    json_decref(array);
    return NULL;
}

static json_t *parse_value(lex_t *lex, json_error_t *error)
{
    json_t *json;

    switch(lex->token) {
        case TOKEN_STRING: {
            json = json_string_nocheck(lex->value.string);
            break;
        }

        case TOKEN_INTEGER: {
            json = json_integer(lex->value.integer);
            break;
        }

        case TOKEN_REAL: {
            json = json_real(lex->value.real);
            break;
        }

        case TOKEN_TRUE:
            json = json_true();
            break;

        case TOKEN_FALSE:
            json = json_false();
            break;

        case TOKEN_NULL:
            json = json_null();
            break;

        case '{':
            json = parse_object(lex, error);
            break;

        case '[':
            json = parse_array(lex, error);
            break;

        case TOKEN_INVALID:
            error_set(error, lex, "invalid token");
            return NULL;

        default:
            error_set(error, lex, "unexpected token");
            return NULL;
    }

    if(!json)
        return NULL;

    return json;
}

static json_t *parse_json(lex_t *lex, json_error_t *error)
{
    error_init(error);

    lex_scan(lex, error);
    if(lex->token != '[' && lex->token != '{') {
        error_set(error, lex, "'[' or '{' expected");
        return NULL;
    }

    return parse_value(lex, error);
}

struct string_data_t {
    const char *data;
    int pos;
};

static int string_get(void *data)
{
    char c;
    string_data_t *stream = (string_data_t *)data;
    c = stream->data[stream->pos];
    if(c == '\0')
        return EOF;
    else
    {
        stream->pos++;
        return c;
    }
}

static int string_eof(void *data)
{
    string_data_t *stream = (string_data_t *)data;
    return (stream->data[stream->pos] == '\0');
}

json_t *json_loads(const char *string, json_error_t *error) {
    lex_t lex;
    json_t *result;

    string_data_t stream_data = {
        .data = string,
        .pos = 0
    };

    if(lex_init(&lex, string_get, string_eof, (void *)&stream_data))
        return NULL;

    result = parse_json(&lex, error);
    if(!result)
        goto out;

    lex_scan(&lex, error);
    if(lex.token != TOKEN_EOF) {
        error_set(error, &lex, "end of file expected");
        json_decref(result);
        result = NULL;
    }

out:
    lex_close(&lex);
    return result;
}

json_t *json_loadf(FILE *input, json_error_t *error)
{
    lex_t lex;
    json_t *result;

    if(lex_init(&lex, (get_func)fgetc, (eof_func)feof, input))
        return NULL;

    result = parse_json(&lex, error);
    if(!result)
        goto out;

    lex_scan(&lex, error);
    if(lex.token != TOKEN_EOF) {
        error_set(error, &lex, "end of file expected");
        json_decref(result);
        result = NULL;
    }

out:
    lex_close(&lex);
    return result;
}

json_t *json_load_file(const char *path, json_error_t *error)
{
    json_t *result;
    FILE *fp;

    error_init(error);

    fp = fopen(path, "r");
    if(!fp)
    {
        error_set(error, NULL, "unable to open %s: %s",
                  path, strerror(errno));
        return NULL;
    }

    result = json_loadf(fp, error);

    fclose(fp);
    return result;
}

/* load end */