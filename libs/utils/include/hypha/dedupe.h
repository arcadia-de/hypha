#ifndef HYPHA_DEDUPE_H
#define HYPHA_DEDUPE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void DedupeCStringsWithComparator(char** arr, size_t size, size_t* new_size,
                                                int (*compare)(const char*, const char*), void (*free_data)(void*)) {
  if (arr == NULL || size == 0)
    return;

  for (size_t i = 0; i < size; i++) {
    for (size_t j = i + 1; j < size; j++) {
      if (compare(arr[i], arr[j]) == 0) {
        free_data(arr[j]);
        arr[j] = arr[size - 1];
        size--;
        j--;
      }
    }
  }

  (*new_size) = size;
}

static inline void DedupeCStrings(char** arr, size_t size, size_t* new_size) {
  return DedupeCStringsWithComparator(arr, size, new_size, &strcmp, free);
}

#endif  // HYPHA_DEDUPE_H
