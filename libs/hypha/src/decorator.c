#include "hypha/decorator.h"

#include <stdlib.h>
#include <string.h>

#include "hypha/log.h"
#include "hypha/resource.h"

typedef struct {
  ResourceDecoratorFn fn;
  void* data;
  void (*free_data)(void* data);
} ResourceDecoratorPipelineStage;

struct _ResourceDecoratorPipeline {
  ResourceDecoratorPipelineStage* stages;
  uint64_t stages_len;
  uint64_t stages_cap;
};

#define BEGIN_FOREACH_DECORATOR_PIPELINE_STAGE(Pipeline, Stage) \
  for (int i = 0; i < (Pipeline)->stages_len; i++) {            \
    ResourceDecoratorPipelineStage* Stage = &(Pipeline)->stages[i];

#define END_FOREACH_DECORATOR_PIPELINE_STAGE }

static inline ResourceDecoratorPipelineStage* NewResourceDecoratorPipelineStage(ResourceDecoratorPipeline* pipe) {
  if (!pipe)
    return NULL;

  if ((pipe->stages_len + 1) >= pipe->stages_cap) {
    const uint64_t new_cap = pipe->stages_cap * 2;
    ResourceDecoratorPipelineStage* new_stages = (ResourceDecoratorPipelineStage*)realloc(pipe->stages, new_cap);
    if (!new_stages) {
      LOG_FATAL("failed to re-allocate %lu new ResourceDecoratorPipeline stages", new_cap);
      return NULL;
    }

    pipe->stages = new_stages;
    pipe->stages_cap = new_cap;
  }

  ResourceDecoratorPipelineStage* next = &pipe->stages[pipe->stages_len];
  pipe->stages_len++;
  return next;
}

void ResourceDecoratorPipelineAdd(ResourceDecoratorPipeline* pipe, ResourceDecoratorFn fn, void* data,
                                  void (*free_data)(void*)) {
  ResourceDecoratorPipelineStage* stage = NewResourceDecoratorPipelineStage(pipe);
  if (!stage) {
    DLOG_ERROR("failed to add new ResourceDecoratorPipelineStage");
    return;
  }

  stage->fn = fn;
  stage->data = data;
  stage->free_data = free_data;
}

static inline void ResourceDecoratorPipelineAddAnnotation(Resource* res, void* data) {
  ResourceAnnotation* annotation = (ResourceAnnotation*)data;
  if (!annotation)
    return;

  PushResourceAnnotation(res, annotation);
  DeleteResourceAnnotation(annotation);
}

void ResourceDecoratorPipelineAnnotate(ResourceDecoratorPipeline* pipe, const char* name, const char* value) {
  return ResourceDecoratorPipelineAdd(pipe, &ResourceDecoratorPipelineAddAnnotation, NewResourceAnnotation(name, value),
                                      (void (*)(void*))FreeResourceAnnotation);
}

static inline void ResourceDecoratorPipelineAddLabel(Resource* res, void* data) {
  ResourceInfo* info = &res->info;
  info->labels[info->labels_len] = strdup((char*)data);
  info->labels_len++;
}

void ResourceDecoratorPipelineLabel(ResourceDecoratorPipeline* pipe, const char* label) {
  return ResourceDecoratorPipelineAdd(pipe, &ResourceDecoratorPipelineAddLabel, (void*)label, NULL);
}

static inline void ExecStage(ResourceDecoratorPipelineStage* stage, Resource* res) {
  return stage->fn(res, stage->data);
}

void ResourceDecoratorPipelineExec(ResourceDecoratorPipeline* pipe, Resource* res) {
  BEGIN_FOREACH_DECORATOR_PIPELINE_STAGE(pipe, stage)
  ExecStage(stage, res);
  END_FOREACH_DECORATOR_PIPELINE_STAGE
}

ResourceDecoratorPipeline* NewResourceDecoratorPipeline() {
  ResourceDecoratorPipeline* pipe = (ResourceDecoratorPipeline*)malloc(sizeof(ResourceDecoratorPipeline));
  if (!pipe)
    return NULL;  // NOLINT(modernize-use-nullptr)

  memset(pipe, 0, sizeof(ResourceDecoratorPipeline));
  return pipe;
}

void FreeResourceDecoratorPipeline(ResourceDecoratorPipeline* pipe) {
  if (!pipe)
    return;

  BEGIN_FOREACH_DECORATOR_PIPELINE_STAGE(pipe, stage)
  if (stage->free_data)
    stage->free_data(stage->data);
  END_FOREACH_DECORATOR_PIPELINE_STAGE

  free(pipe->stages);
  free(pipe);
}
