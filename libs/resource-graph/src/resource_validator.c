#include "hypha/resource_validator.h"

#include "hypha/assertions.h"

struct _ResourceValidator {
  ResourceValidator* next;
  ResourceValidatorFn fn;
  void* data;
  void (*free_data)(void*);
};

static inline bool HasNext(ResourceValidator* rhs) {
  ASSERT(rhs);
  return rhs->next;
}

static inline ResourceValidator* GetNext(ResourceValidator* rhs) {
  ASSERT(HasNext(rhs));
  return rhs->next;
}

static inline ResourceValidator* GetLast(ResourceValidator* rhs) {
  ResourceValidator* last = rhs;
  while (HasNext(last))
    last = GetNext(last);
  return last;
}

ResourceValidator* NewResourceValidator(ResourceValidatorFn fn, void* data, void (*free_data)(void*)) {
  ResourceValidator* rv = (ResourceValidator*)malloc(sizeof(ResourceValidator));
  if (rv) {
    memset(rv, 0, sizeof(ResourceValidator));
    rv->fn = fn;
    rv->next = NULL;
    rv->data = data;
    rv->free_data = free_data;
  }
  return rv;
}

void ResourceValidatorLink(ResourceValidator** head, ResourceValidator* rhs) {
  ASSERT(rhs);
  if ((*head) == NULL) {
    (*head) = rhs;
    return;
  }

  ResourceValidator* last = GetLast(*head);
  last->next = rhs;
}

bool ValidateResource(ResourceValidator* rv, const Resource* res) {
  bool valid = false;
  if (!rv || !res)
    goto finished;

  ResourceValidator* current = rv;
  while (current != NULL) {
    if (!current->fn(res, current->data))
      goto finished;

    current = current->next;
  }

  valid = true;
finished:
  return valid;
}

void FreeResourceValidator(ResourceValidator* validator) {
  if (!validator)
    return;

  if (HasNext(validator))
    FreeResourceValidator(GetNext(validator));

  if (validator->data && validator->free_data)
    validator->free_data(validator->data);

  free(validator);
}
