#ifndef HYPHA_ANNOTATION_H
#define HYPHA_ANNOTATION_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <string.h>

#ifndef HYPHA_ANNOTATION_KEY_SIZE
#define HYPHA_ANNOTATION_KEY_SIZE 64
#endif  // HYPHA_ANNOTATION_KEY_SIZE

#ifndef HYPHA_ANNOTATION_VALUE_SIZE
#define HYPHA_ANNOTATION_VALUE_SIZE 256
#endif  // HYPHA_ANNOTATION_VALUE_SIZE

typedef char AnnotationKey[HYPHA_ANNOTATION_KEY_SIZE];

static inline int CompareAnnotationKey(const AnnotationKey* lhs, const AnnotationKey* rhs) {
  return strncmp(*lhs, *rhs, HYPHA_ANNOTATION_KEY_SIZE);
}

static inline bool AnnotationKeyEq(const AnnotationKey* lhs, const AnnotationKey* rhs) {
  return CompareAnnotationKey(lhs, rhs) == 0;
}

static inline bool AnnotationKeyEqCStr(const AnnotationKey* lhs, const char* rhs) {
  if (strlen(rhs) != HYPHA_ANNOTATION_KEY_SIZE)
    return false;
  return strncmp(*lhs, rhs, HYPHA_ANNOTATION_KEY_SIZE) == 0;
}

typedef char AnnotationValue[HYPHA_ANNOTATION_VALUE_SIZE];

static inline int CompareAnnotationValue(const AnnotationValue* lhs, const AnnotationValue* rhs) {
  return strncmp(*lhs, *rhs, HYPHA_ANNOTATION_VALUE_SIZE);
}

static inline bool AnnotationValueEq(const AnnotationValue* lhs, const AnnotationValue* rhs) {
  return CompareAnnotationValue(lhs, rhs) == 0;
}

static inline bool AnnotationValueEqCStr(const AnnotationValue* lhs, const char* rhs) {
  if (strlen(rhs) != HYPHA_ANNOTATION_VALUE_SIZE)
    return false;
  return strncmp(*lhs, rhs, HYPHA_ANNOTATION_VALUE_SIZE) == 0;
}

typedef struct {
  AnnotationKey key;
  AnnotationValue value;
} Annotation;

void InitAnnotation(Annotation* annotation, const AnnotationKey key, const AnnotationValue value);
Annotation* NewAnnotation(const AnnotationKey, const AnnotationValue);
void FreeAnnotation(Annotation*);

static inline int CompareAnnotationByKey(const Annotation* lhs, const AnnotationKey* rhs) {
  return CompareAnnotationKey(&lhs->key, rhs);
}

static inline int CompareAnnotationByValue(const Annotation* lhs, const AnnotationValue* rhs) {
  return CompareAnnotationValue(&lhs->value, rhs);
}

static inline bool AnnotationEq(const Annotation* lhs, const Annotation* rhs) {
  return CompareAnnotationByKey(lhs, &rhs->key) == 0 && CompareAnnotationByValue(lhs, &rhs->value) == 0;
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_ANNOTATION_H
