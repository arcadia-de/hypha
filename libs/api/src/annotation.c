#include "hypha/annotation.h"

void InitAnnotation(Annotation* annotation, const AnnotationKey key, const AnnotationValue value);
Annotation* NewAnnotation(const AnnotationKey, const AnnotationValue);
void FreeAnnotation(Annotation*);
