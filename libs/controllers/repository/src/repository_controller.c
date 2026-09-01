#include "hypha/repository_controller.h"

#include <git2.h>
#include <git2/clone.h>
#include <jansson.h>
#include <sys/stat.h>

#include "hypha.h"
#include "hypha/action_log.h"
#include "hypha/controller_status.h"
#include "hypha/expander.h"
#include "hypha/log.h"
#include "hypha/planned_action.h"
#include "hypha/planner.h"
#include "hypha/reason.h"
#include "hypha/repository_spec.h"
#include "hypha/validation_log.h"

static inline void InitController(void* data) {
  git_libgit2_init();
}

static inline void DeInitController(void* data) {
  git_libgit2_shutdown();
}

static inline bool GetSpecField(const Resource* res, const char* field, char** result, size_t* result_len) {
  ASSERT(res);
  ASSERT(res->spec.doc);

  json_t* source = json_object_get(res->spec.doc, field);
  if (!source || !json_is_string(source))
    return false;

  Expander expander;
  return ExpandStr(&expander, json_string_value(source), result, result_len);
}

// Returns true if `dir` is already the workdir of a git repository. Deliberately uses
// GIT_REPOSITORY_OPEN_NO_SEARCH so a destination nested inside some *other* repo isn't
// mistaken for one of its own -- we only care whether `dir` itself has been cloned into.
static inline bool DirectoryHasGitRepo(const char* dir) {
  git_repository* repo = NULL;
  const int err = git_repository_open_ext(&repo, dir, GIT_REPOSITORY_OPEN_NO_SEARCH, NULL);
  if (err < 0)
    return false;

  git_repository_free(repo);
  return true;
}

static inline bool Clone(const char* url, const char* dir) {
  bool success = false;
  git_repository* repo = NULL;

  git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
  DLOG_INFO("cloning '%s' into '%s'...", url, dir);
  int err = git_clone(&repo, url, dir, &clone_opts);
  if (err < 0) {
    const git_error* e = git_error_last();
    LOG_ERROR("error %d while cloning repository: %s", err, e ? e->message : "Unknown");
    goto finished;
  }

  DLOG_INFO("clone finished, available at: %s", git_repository_workdir(repo));
  git_repository_free(repo);
  success = true;
finished:
  return success;
}

thread_local RepositorySpec repository_spec;

static const char kUrlField[] = "url";
static const char kDestinationField[] = "destination";

DEFINE_CONTROLLER_OBSERVE_FN(Repository) {
  Resource* observed = ctx->observed;
  ASSERT(observed);

  if (!observed->spec.doc)
    return kStatusOk;

  if (!GetSpecField(observed, kUrlField, &repository_spec.url, &repository_spec.url_len)) {
    LOG_ERROR("failed to get `%s` field", kUrlField);
    return kStatusInternalError;
  }

  if (!GetSpecField(observed, kDestinationField, &repository_spec.destination, &repository_spec.destination_len)) {
    LOG_ERROR("failed to get `%s` field", kDestinationField);
    return kStatusInternalError;
  }

  return kStatusOk;
}

DEFINE_CONTROLLER_VALIDATE_FN(Repository) {
  ValidationLog* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (!repository_spec.url || repository_spec.url_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` repository_spec field", kUrlField);
    return false;
  }

  if (!repository_spec.destination || repository_spec.destination_len == 0) {
    NewFailedValidationResult(log, desired, "Failed to get `%s` repository_spec field", kDestinationField);
    return false;
  }

  NewPassedValidationResult(log, desired, "Spec is valid");
  return true;
}

DEFINE_CONTROLLER_PLAN_FN(Repository) {
  Plan* log = ctx->log;
  ASSERT(log);
  Resource* desired = (Resource*)ctx->desired;  // TODO(@s0cks): const cast
  ASSERT(desired);

  if (DirectoryHasGitRepo(repository_spec.destination)) {
    PlannedAction* action =
        NewNoPlannedAction(log, desired, "Destination `%s` is already a git repository", repository_spec.destination);
    // TODO(@s0cks): this only checks "something got cloned here" -- a fuller Plan would also
    // compare the current remote/HEAD against `repository_spec.url` and report an Update
    // action when they've drifted, but there's nothing here yet to act on that (no fetch,
    // no checkout). Scoped to "clone if not already cloned," not "keep in sync."
    ASSERT(action);
    return kNoAction;
  }

  struct stat dest_stat;
  if (stat(repository_spec.destination, &dest_stat) == 0) {
    PlannedAction* action = NewNoPlannedAction(log, desired, "Destination `%s` exists but is not a git repository",
                                                repository_spec.destination);
    ASSERT(action);
    return kNoAction;
  }

  PlannedAction* action =
      NewCreatePlannedAction(log, desired, "Destination `%s` doesn't exist", repository_spec.destination);
  ASSERT(action);
  return kCreateAction;
}

DEFINE_CONTROLLER_APPLY_FN(Repository) {
  Resource* desired = ctx->desired;
  ASSERT(desired);

  if (!Clone(repository_spec.url, repository_spec.destination)) {
    LOG_ERROR("failed to clone `%s` into `%s`", repository_spec.url, repository_spec.destination);
    return kStatusInternalError;
  }

  AppliedAction* action = NewCreateAction(ctx->log, desired, "Repository `%s` cloned to `%s`", repository_spec.url,
                                          repository_spec.destination);
  ASSERT(action);
  return kStatusOk;
}

DEFINE_CONTROLLER_STATUS_FN(Repository) {
  const Resource* current = ctx->current;
  ASSERT(current);

  if (!DirectoryHasGitRepo(repository_spec.destination)) {
    LOG_ERROR("Destination `%s` is not a git repository", repository_spec.destination);
    return kStatusInternalError;
  }

  return kStatusOk;
}

static const ControllerConfig kRepositoryControllerConfig = {
    .init = InitController,
    .deinit = DeInitController,
    .observe = RepositoryObserve,
    .plan = RepositoryPlan,
    .apply = RepositoryApply,
    .validate = RepositoryValidate,
    .diff = NULL,
    .status = RepositoryStatus,
    .rollback = NULL,
    .normalize = NULL,
    .destroy = NULL,
};
DEFINE_NEW_CONTROLLER(Repository);
