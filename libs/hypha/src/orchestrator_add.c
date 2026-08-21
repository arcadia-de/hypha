#include <xxhash.h>

#include "hypha/orchestrator.h"
#include "orc.h"

void OrchestratorAddResource(OrchestratorHandle handle, Resource* res) {
  if (!handle)
    return;

  Orchestrator* orc = (Orchestrator*)handle;
  Resource* new_res = AllocNewResouceInGraph(orc->graph);
  ResourceInfo* source_info = &res->info;
  ResourceInfo* dest_info = &new_res->info;

  new_res->id = strdup(res->id);
  if (!new_res->id)
    return;  // TODO(@s0cks): probably should reclaim the allocated id

  new_res->kind = strdup(res->kind);
  if (!new_res->kind)
    return;  // TODO(@s0cks): probably should reclaim the allocated id

  if (res->spec.raw) {
    new_res->spec.raw = strdup(res->spec.raw);
    if (!new_res->spec.raw)
      return;  // TODO(@s0cks): probably should reclaim the allocated id
    new_res->spec.doc = NULL;
    new_res->spec.hash = XXH3_64bits(new_res->spec.raw, strlen(new_res->spec.raw));
  }

  if (source_info->labels_len > 0) {
    const size_t total_size = sizeof(Label) * source_info->labels_len;
    Label* new_labels = (Label*)malloc(total_size);
    LOG_FATAL_IF(!new_labels, "failed to allocate new labels for new resource");
    memset(new_labels, 0, total_size);
    memcpy(new_labels, source_info->labels, total_size);

    dest_info->labels = new_labels;
    dest_info->labels_len = dest_info->labels_cap = source_info->labels_len;
  } else {
    dest_info->labels = NULL;
    dest_info->labels_cap = dest_info->labels_len = 0;
  }

  if (source_info->annotations_len > 0) {
    const size_t total_size = sizeof(Annotation) * source_info->annotations_len;
    Annotation* new_annotations = (Annotation*)malloc(total_size);
    LOG_FATAL_IF(!new_annotations, "failed to allocate new annotations for new resource");
    memset(new_annotations, 0, total_size);
    memcpy(new_annotations, source_info->annotations, total_size);

    dest_info->annotations = new_annotations;
    dest_info->annotations_len = dest_info->annotations_cap = source_info->annotations_len;
  } else {
    dest_info->annotations = NULL;
    dest_info->annotations_cap = dest_info->annotations_len = 0;
  }

  new_res->state = kResourcePending;
  new_res->num_depends_on = res->num_depends_on;

  if (res->num_depends_on > 0) {
    char** depends_on = (char**)malloc(sizeof(char*) * res->num_depends_on);
    if (!depends_on)
      return;  // TODO(@s0cks): probably should reclaim the allocated id

    for (int i = 0; i < res->num_depends_on; i++)
      depends_on[i] = strdup(res->depends_on[i]);

    new_res->depends_on = depends_on;
  } else {
    new_res->depends_on = NULL;  // NOLINT(modernize-use-nullptr)
  }
}
