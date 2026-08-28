#include "hypha/label.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

Label* labels = NULL;
size_t labels_len = 0;
size_t labels_cap = 0;

static inline void EnsureCap(const size_t new_len) {
  if (new_len < labels_cap)
    return;

  const size_t new_cap = (labels_cap + new_len + 1);
  const size_t total_size = sizeof(Label) * new_cap;
  Label* new_labels = (Label*)realloc(labels, total_size);
  if (!new_labels)
    return;

  labels = new_labels;
  labels_cap = new_cap;
}

const Label* GetDefaultLabels() {
  return labels;
}

size_t GetNumberOfDefaultLabels() {
  return labels_len;
}

void AppendDefaultLabels(const Label* rhs, const size_t len) {
  EnsureCap(labels_len + len);
  Label* dst = &labels[labels_len];
  labels_len += len;
  memcpy(dst, rhs, sizeof(Label) * len);
}

void VisitAllDefaultLabels(VisitLabelFn fn, void* data) {
  for (size_t i = 0; i < labels_len; i++) {
    if (!fn(i, labels[i], data))
      return;
  }
}

void FreeDefaultLabels() {
  if (!labels || labels_cap == 0)
    return;

  free(labels);
}
