#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha/query.h"

void ResultNodeFree(QueryResult* node) {
  if (!node)
    return;

  switch (node->kind) {
    case kQueryResultString:
      free(node->string_value);
      break;
    case kQueryResultObject:
      for (uint32_t i = 0; i < node->num_object_fields; i++) {
        free(node->object_fields[i].key);
        ResultNodeFree(node->object_fields[i].value);
      }

      free(node->object_fields);
      break;
    case kQueryResultArray:
      for (uint32_t i = 0; i < node->num_array_items; i++)
        ResultNodeFree(node->array_items[i]);

      free(node->array_items);
      break;
    case kQueryResultNull:
      break;
    default:
      break;
  }

  free(node);
}

static void AppendEscapedString(char** buf, uint32_t* len, uint32_t* cap, const char* s) {
#define ENSURE(extra)                    \
  do {                                   \
    if (*len + (extra) + 1 > *cap) {     \
      *cap = (*len + (extra) + 1) * 2;   \
      *buf = (char*)realloc(*buf, *cap); \
    }                                    \
  } while (0)

  ENSURE(1);
  (*buf)[(*len)++] = '"';

  for (const char* p = s; *p; p++) {
    ENSURE(2);
    switch (*p) {
      case '"':
        (*buf)[(*len)++] = '\\';
        (*buf)[(*len)++] = '"';
        break;
      case '\\':
        (*buf)[(*len)++] = '\\';
        (*buf)[(*len)++] = '\\';
        break;
      case '\n':
        (*buf)[(*len)++] = '\\';
        (*buf)[(*len)++] = 'n';
        break;
      case '\t':
        (*buf)[(*len)++] = '\\';
        (*buf)[(*len)++] = 't';
        break;
      default:
        if ((unsigned char)*p < 0x20) {
          char esc[8];
          snprintf(esc, sizeof(esc), "\\u%04x", *p);
          ENSURE(6);
          memcpy(*buf + *len, esc, 6);
          *len += 6;
        } else {
          (*buf)[(*len)++] = *p;
        }
    }
  }

  ENSURE(1);
  (*buf)[(*len)++] = '"';
  (*buf)[*len] = '\0';

#undef ENSURE
}

static void AppendRaw(char** buf, uint32_t* len, uint32_t* cap, const char* s) {
  const uint32_t n = (uint32_t)strlen(s);
  if (*len + n + 1 > *cap) {
    *cap = (*len + n + 1) * 2;
    *buf = (char*)realloc(*buf, *cap);
  }

  memcpy(*buf + *len, s, n);
  *len += n;
  (*buf)[*len] = '\0';
}

static void SerializeNode(const QueryResult* node, char** buf, uint32_t* len, uint32_t* cap) {
  switch (node->kind) {
    case kQueryResultNull:
      AppendRaw(buf, len, cap, "null");
      break;
    case kQueryResultString:
      AppendEscapedString(buf, len, cap, node->string_value);
      break;
    case kQueryResultArray:
      AppendRaw(buf, len, cap, "[");
      for (uint32_t i = 0; i < node->num_array_items; i++) {
        if (i > 0)
          AppendRaw(buf, len, cap, ",");

        SerializeNode(node->array_items[i], buf, len, cap);
      }

      AppendRaw(buf, len, cap, "]");
      break;
    case kQueryResultObject:
      AppendRaw(buf, len, cap, "{");
      for (uint32_t i = 0; i < node->num_object_fields; i++) {
        if (i > 0)
          AppendRaw(buf, len, cap, ",");

        AppendEscapedString(buf, len, cap, node->object_fields[i].key);
        AppendRaw(buf, len, cap, ":");
        SerializeNode(node->object_fields[i].value, buf, len, cap);
      }

      AppendRaw(buf, len, cap, "}");
      break;
    default:
      break;
  }
}

char* ResultNodeToJSON(const QueryResult* node) {
  uint32_t len = 0, cap = 64;
  char* buf = (char*)malloc(cap);
  buf[0] = '\0';

  SerializeNode(node, &buf, &len, &cap);
  return buf;
}
