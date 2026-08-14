#include "repository_controller.h"

#include <git2.h>
#include <git2/clone.h>

#include "hypha.h"
#include "hypha/log.h"

static inline void InitController(void* data) {
  git_libgit2_init();
}

static inline void DeInitController(void* data) {
  git_libgit2_shutdown();
}

static inline bool Clone(const char* url, const char* dir) {
  bool success = false;
  git_repository* repo = NULL;

  git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
  DLOG_INFO("cloning '%s' into '%s'...", url, dir);
  int err = git_clone(&repo, url, dir, &clone_opts);
  if (err < 0) {
    const git_error* e = git_error_last();
    LOG_ERROR("error %d while cloning report: %s", err, e ? e->message : "Unknown");
    goto finished;
  }

  DLOG_INFO("clone finished, available at: %s", git_repository_workdir(repo));
  git_repository_free(repo);
  success = true;
finished:
  return success;
}

DEFINE_CONTROLLER_OBSERVE_FN(Repository) {
  // if (!desired->spec)
  //   return kStatusOk;  // nothing declared to check against -- Plan/Apply report the real error
  //
  // char* destination = ExtractJsonStringField(desired->spec, "destination");
  // if (!destination)
  //   return kStatusOk;
  //
  // char* expanded = ExpandHomePath(destination);
  // if (DirectoryHasGit(expanded)) {
  //   // Signal "found something" the minimal way Plan below actually
  //   // checks for -- a fuller Observe would also read the current
  //   // remote/HEAD and hand it back for Plan to compare against
  //   // desired, but there's nothing in Resource yet to carry that
  //   // structured comparison, and no Update/Destroy logic implemented
  //   // here to act on it yet either. This is honestly scoped to
  //   // "clone if not already cloned," not "keep in sync."
  //   out->id = strdup(desired->id);
  // }
  //
  // free(expanded);
  // free(destination);
  return kStatusOk;
}

DEFINE_CONTROLLER_PLAN_FN(Repository) {
  return kNoAction;
}

DEFINE_CONTROLLER_APPLY_FN(Repository) {
  return kStatusOk;
}

static const ControllerConfig kRepositoryControllerConfig = {
    .init = InitController,
    .deinit = DeInitController,
    .observe = RepositoryObserve,
    .plan = RepositoryPlan,
    .apply = RepositoryApply,
};
DEFINE_NEW_CONTROLLER(Repository, REPOSITORY_CONTROLLER_KIND);
