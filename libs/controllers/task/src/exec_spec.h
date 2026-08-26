#ifndef HYPHA_EXEC_SPEC_H
#define HYPHA_EXEC_SPEC_H

#include "hypha/task_spec.h"

void ParseExecSpec(json_t* doc, ExecSpec* spec);
const char* ResolveShellPath(const ExecSpec* spec);

#endif  // HYPHA_EXEC_SPEC_H
