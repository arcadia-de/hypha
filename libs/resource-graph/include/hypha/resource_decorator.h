#ifndef HYPHA_DECORATOR_H
#define HYPHA_DECORATOR_H

#include "hypha.h"
#include "hypha/resource.h"

typedef void (*ResourceDecoratorFn)(Resource* res, void* data);

typedef struct {
  ResourceDecoratorFn fn;
  void* data;
  void (*free_data)(void* data);
} ResourceDecoratorPipelineStage;

typedef struct {
  ResourceDecoratorPipelineStage* stages;
  size_t stages_len;
  size_t stages_cap;
} ResourceDecoratorPipeline;

void InitResourceDecoratorPipeline(ResourceDecoratorPipeline* pipe, const size_t init_cap);
void ResourceDecoratorPipelineAdd(ResourceDecoratorPipeline* pipe, ResourceDecoratorFn fn, void* data,
                                  void (*free_data)(void*));
void ResourceDecoratorPipelineLabel(ResourceDecoratorPipeline* pipe, const Label* label);
void ResourceDecoratorPipelineAnnotate(ResourceDecoratorPipeline* pipe, const AnnotationKey* k,
                                       const AnnotationValue* v);
void ResourceDecoratorPipelineExec(ResourceDecoratorPipeline* pipe, Resource* res);
void FreeResourceDecoratorPipeline(ResourceDecoratorPipeline* pipe);

// TODO(@s0cks): ????
//  Same as Add, but the pipeline takes ownership of `data`: `free_data`
//  (if non-NULL) is called on it exactly once, when the pipeline itself
//  is freed — not after each resource the stage runs against, since a
//  single stage runs once per resource across the whole graph. Used by
//  Label/Annotate/LabelKind/AnnotateKind below for the copies they make;
//  available directly for custom stages that allocate their own data.
void ResourceDecoratorPipelineAddOwned(ResourceDecoratorPipeline* pipe, ResourceDecoratorFn fn, void* data,
                                       void (*free_data)(void*));

#endif  // HYPHA_DECORATOR_H
