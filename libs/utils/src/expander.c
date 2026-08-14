#include "hypha/expander.h"

#include <ctype.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "expander_buffer.h"
#include "stdio.h"

static inline int IsIdentChar(char c) {
  return isalnum((unsigned char)c) || c == '_';
}

static inline const char* GetHomeDir(void) {
  const char* home = getenv("HOME");
  if (home && *home)
    return home;

  struct passwd* pw = getpwuid(getuid());
  if (pw && pw->pw_dir)
    return pw->pw_dir;

  return NULL;
}

static inline const char* GetHomeDirForUser(const char* name, size_t namelen) {
  if (namelen == 0)
    return GetHomeDir();

  char namebuf[256];
  if (namelen >= sizeof(namebuf))
    return NULL;

  memcpy(namebuf, name, namelen);
  namebuf[namelen] = '\0';

  struct passwd* pw = getpwnam(namebuf);
  if (pw && pw->pw_dir)
    return pw->pw_dir;

  return NULL;
}

static inline void ExpandVar(ExpanderBuffer* buff, const char* input, size_t* pi, size_t len) {
  size_t i = *pi;

  if (i < len && input[i] == '{') {
    size_t start = i + 1;
    size_t j = start;
    int depth = 1;
    while (j < len && depth > 0) {
      if (input[j] == '{') {
        depth++;
      } else if (input[j] == '}') {
        depth--;
        if (depth == 0)
          break;
      }

      j++;
    }

    if (j >= len) {
      ExBuffAppendChar(buff, '$');
      *pi = i;
      return;
    }

    const char* body = input + start;
    size_t body_len = j - start;

    size_t colon = (size_t)-1;
    for (size_t k = 0; k < body_len; k++) {
      if (body[k] == ':') {
        colon = k;
        break;
      }
    }

    size_t name_len = (colon == (size_t)-1) ? body_len : colon;
    char namebuf[256];
    if (name_len >= sizeof(namebuf))
      name_len = sizeof(namebuf) - 1;

    memcpy(namebuf, body, name_len);
    namebuf[name_len] = '\0';

    const char* def = NULL;
    size_t def_len = 0;
    if (colon != (size_t)-1) {
      size_t def_start = colon + 1;
      if (def_start < body_len && body[def_start] == '-')
        def_start++;

      def = body + def_start;
      def_len = (def_start <= body_len) ? body_len - def_start : 0;
    }

    const char* val = getenv(namebuf);
    if (val && *val) {
      ExBuffAppendStr(buff, val);
    } else if (def) {
      ExBuffAppend(buff, def, def_len);
    }

    *pi = j + 1;
    return;
  }

  size_t start = i;
  while (i < len && IsIdentChar(input[i]))
    i++;

  if (i == start) {
    ExBuffAppendChar(buff, '$');
    *pi = start;
    return;
  }

  char namebuf[256];
  size_t name_len = i - start;
  if (name_len >= sizeof(namebuf))
    name_len = sizeof(namebuf) - 1;
  memcpy(namebuf, input + start, name_len);
  namebuf[name_len] = '\0';

  const char* val = getenv(namebuf);
  if (val)
    ExBuffAppendStr(buff, val);

  *pi = i;
}

static inline const char* ExpandSymbol(Expander* expander, const char sym) {
  return expander->resolve ? expander->resolve(sym, expander->data) : NULL;
}

bool Expand(Expander* expander, const char* value, const size_t value_len, char** result, size_t* result_len) {
  bool success = false;
  if (!expander || !value || value_len == 0)
    goto finished;

  ExpanderBuffer buff;
  InitExBuffer(&buff, 0);

  size_t i = 0;
  if (value_len > 0 && value[0] == '~') {
    size_t j = 1;
    while (j < value_len && value[j] != '/' && value[j] != '\0')
      j++;

    const char* home = GetHomeDirForUser(value + 1, j - 1);
    if (home) {
      ExBuffAppendStr(&buff, home);
      i = j;
    }
  } else if (value_len > 0 && value[0] == '.') {
    char* cwd = getcwd(NULL, 0);
    if (!cwd)
      goto finished;
    ExBuffAppendStr(&buff, cwd);
    i++;
    free(cwd);
  }

  for (; i < value_len; i++) {
    char c = value[i];

    if (c == '\\' && i + 1 < value_len) {
      ExBuffAppendChar(&buff, value[i + 1]);
      i++;
      continue;
    }

    if (c == '$') {
      i++;
      ExpandVar(&buff, value, &i, value_len);
      i--;
      continue;
    }

    if (c == '%' && i + 1 < value_len) {
      char sym = value[i + 1];
      if (sym == 'h') {
        char resolved[PATH_MAX];
        snprintf(resolved, PATH_MAX, "%s/.config/hypha", GetHomeDir());
        ExBuffAppendStr(&buff, resolved);
      } else {
        const char* resolved = ExpandSymbol(expander, sym);
        if (resolved) {
          ExBuffAppendStr(&buff, resolved);
        } else {
          ExBuffAppendChar(&buff, '%');
          ExBuffAppendChar(&buff, sym);
        }
      }

      i++;
      continue;
    }

    ExBuffAppendChar(&buff, c);
  }

  (*result) = buff.data;
  (*result_len) = buff.len;
  success = true;
finished:
  return success;
}
