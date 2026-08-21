#include "hypha/resource_decorator.h"

#include <stdlib.h>
#include <string.h>

#include "hypha/label.h"
#include "hypha/log.h"
#include "hypha/resource.h"

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
  Annotation* annotation = (Annotation*)data;
  if (!annotation)
    return;

  PushResourceAnnotation(res, annotation);
}

void ResourceDecoratorPipelineAnnotate(ResourceDecoratorPipeline* pipe, const AnnotationKey* key,
                                       const AnnotationValue* value) {
  Annotation annotation;
  memcpy(annotation.key, *key, sizeof(AnnotationKey));
  memcpy(annotation.value, *value, sizeof(AnnotationValue));
  return ResourceDecoratorPipelineAdd(pipe, &ResourceDecoratorPipelineAddAnnotation, &annotation, NULL);
}

static inline void ResourceDecoratorPipelineAddLabel(Resource* res, void* data) {
  ResourceInfo* info = &res->info;
  const Label* label = (const Label*)data;

  if (info->labels_len + 1 >= info->labels_cap) {
    const size_t new_cap = info->labels_cap * 2;
    const size_t total_size = sizeof(Label) * new_cap;
    Label* new_labels = (Label*)realloc(info->labels, total_size);
    LOG_FATAL_IF(!new_labels, "failed to allocate new labels for resource");
    info->labels = new_labels;
    info->labels_cap = new_cap;
  }

  memcpy(&info->labels[info->labels_len], label, HYPHA_LABEL_MAX_SIZE);
  info->labels_len++;
}

void ResourceDecoratorPipelineLabel(ResourceDecoratorPipeline* pipe, const Label* label) {
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
