#ifndef HYPHA_LABEL_H
#define HYPHA_LABEL_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <string.h>

#ifndef HYPHA_LABEL_MAX_SIZE
#define HYPHA_LABEL_MAX_SIZE 64
#endif  // HYPHA_LABEL_MAX_SIZE

typedef char Label[HYPHA_LABEL_MAX_SIZE];

typedef bool (*VisitLabelFn)(uint64_t, const Label, void*);

static inline int CompareLabel(const Label lhs, const Label rhs) {
  return strncmp(lhs, rhs, HYPHA_LABEL_MAX_SIZE);
}

static inline bool LabelEq(const Label lhs, const Label rhs) {
  return CompareLabel(lhs, rhs) == 0;
}

const Label* GetDefaultLabels();
size_t GetNumberOfDefaultLabels();
void AppendDefaultLabels(const Label* rhs, const size_t len);
void VisitAllDefaultLabels(VisitLabelFn fn, void* data);
void FreeDefaultLabels();

static inline void AppendDefaultLabel(const Label label) {
  return AppendDefaultLabels((const Label*)&label, 1);
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_LABEL_H
