#include <stdlib.h>

#include "hypha.h"
#include "hypha/event.h"
#include "hypha/orchestrator.h"

int main(int argc, char** argv) {
  InitHypha("");

  OrchestratorConfig config = {
      .root = "/home/tazz/.config/hypha",
      .state_dir = "/home/tazz/.local/state/hypha",
      .cache_dir = "/home/tazz/.cache/hypha",
  };
  Orchestrator* orc = NewOrchestrator(config);
  OrchestratorRun(orc, kOrchestratorPlanMode);
  return EXIT_FAILURE;
}
