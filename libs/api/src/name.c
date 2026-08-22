#include "hypha/name.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#define DEFAULT_NAME_POSTFIX_LENGTH 7

// Lowercase alphanumeric, deliberately narrower than nanoid's default alphabet
// (no '_' or '-') since this postfix ends up embedded in a "<kind>-<postfix>"
// name that may itself end up in filenames, labels, or CLI output.
static const char kDefaultNamePostfixAlphabet[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const size_t kDefaultNamePostfixAlphabetLen = sizeof(kDefaultNamePostfixAlphabet) - 1;

char* GenerateDefaultResourceName(const char* kind) {
  if (!kind)
    return NULL;

  const size_t kind_len = strlen(kind);
  char* lower_kind = (char*)malloc(kind_len + 1);
  if (!lower_kind)
    return NULL;
  for (size_t i = 0; i < kind_len; i++)
    lower_kind[i] = (char)tolower((unsigned char)kind[i]);
  lower_kind[kind_len] = '\0';

  // uuid_generate_random gives us 16 bytes of decent entropy for free without
  // pulling in a dedicated nanoid dependency; DEFAULT_NAME_POSTFIX_LENGTH < 16
  // so a single draw covers the whole postfix.
  uuid_t entropy;
  uuid_generate_random(entropy);

  char postfix[DEFAULT_NAME_POSTFIX_LENGTH + 1];
  for (int i = 0; i < DEFAULT_NAME_POSTFIX_LENGTH; i++)
    postfix[i] = kDefaultNamePostfixAlphabet[entropy[i] % kDefaultNamePostfixAlphabetLen];
  postfix[DEFAULT_NAME_POSTFIX_LENGTH] = '\0';

  const size_t out_len = kind_len + 1 + DEFAULT_NAME_POSTFIX_LENGTH + 1;  // kind + '-' + postfix + '\0'
  char* name = (char*)malloc(out_len);
  if (!name) {
    free(lower_kind);
    return NULL;
  }
  snprintf(name, out_len, "%s-%s", lower_kind, postfix);

  free(lower_kind);
  return name;
}
