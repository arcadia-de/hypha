#ifndef HYPHA_RESOURCE_VALIDATOR_H
#define HYPHA_RESOURCE_VALIDATOR_H

#include "hypha/resource.h"

typedef bool (*ResourceValidatorFn)(const Resource*, void* data);

typedef struct _ResourceValidator ResourceValidator;

ResourceValidator* NewResourceValidator(ResourceValidatorFn fn, void* data, void (*free_data)(void*));
void ResourceValidatorLink(ResourceValidator** head, ResourceValidator* validator);
bool ValidateResource(ResourceValidator*, const Resource*);
void FreeResourceValidator(ResourceValidator*);

#endif  // HYPHA_RESOURCE_VALIDATOR_H
