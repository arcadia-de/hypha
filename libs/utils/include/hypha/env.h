#ifndef HYPHA_ENV_H
#define HYPHA_ENV_H

#include <stdint.h>

typedef bool (*EnvVarVisitor)(uint64_t, const char* key, const char* value, void* data);

void VisitAllEnvVars(EnvVarVisitor vis, void* data);
void AppendToEnvVar(const char* k, const char* value);

#endif  // HYPHA_ENV_H
