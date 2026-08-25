#include "hypha/name.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif  // MIN

#define DEFAULT_NAME_POSTFIX_LENGTH 7

static const char kDefaultNamePostfixAlphabet[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static const size_t kDefaultNamePostfixAlphabetLen = sizeof(kDefaultNamePostfixAlphabet) - 1;

void GenerateDefaultResourceName(ResourceKind kind, Name* name) {
  const char* k = FindResourceKindName(kind);
  const size_t kind_len = strlen(k);
  char* lower_kind = (char*)malloc(kind_len + 1);
  if (!lower_kind) {
    memset(*name, 0, sizeof(Name));
    return;
  }

  for (size_t i = 0; i < kind_len; i++)
    lower_kind[i] = (char)tolower((unsigned char)k[i]);
  lower_kind[kind_len] = '\0';

  uuid_t entropy;
  uuid_generate_random(entropy);

  char postfix[DEFAULT_NAME_POSTFIX_LENGTH + 1];
  for (int i = 0; i < DEFAULT_NAME_POSTFIX_LENGTH; i++)
    postfix[i] = kDefaultNamePostfixAlphabet[entropy[i] % kDefaultNamePostfixAlphabetLen];
  postfix[DEFAULT_NAME_POSTFIX_LENGTH] = '\0';

  const size_t out_len = kind_len + 1 + DEFAULT_NAME_POSTFIX_LENGTH + 1;
  snprintf(*name, MIN(out_len, HYPHA_NAME_MAX_SIZE), "%s-%s", lower_kind, postfix);
  free(lower_kind);
}
