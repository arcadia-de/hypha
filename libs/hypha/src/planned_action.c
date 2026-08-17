#include "hypha/planner.h"

void FreePlannedAction(PlannedAction* action) {
  if (!action)
    return;

  if (action->id)
    free(action->id);
}
