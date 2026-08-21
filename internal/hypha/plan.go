package hypha

/*
#cgo pkg-config: hypha-uninstalled

#include <stdlib.h>
#include "hypha/planner.h"
#include "hypha/orchestrator.h"

bool goVisitPlannedActions(size_t idx, PlannedAction* action, void* data);
*/
import "C"

import (
	"runtime"
	"runtime/cgo"
	"strings"
	"unsafe"
)

type Plan struct {
	Handle *C.Plan
}

type PlannedAction struct {
	ID     string `json:"id,omitempty" yaml:"id,omitempty"`
	Action string `json:"action,omitempty" yaml:"action,omitempty"`
	Reason string `json:"reason,omitempty" yaml:"reason,omitempty"`
}

type PlannedActionVisitor func(idx uint64, action PlannedAction) bool

//export goVisitPlannedActions
func goVisitPlannedActions(idx C.size_t, action *C.PlannedAction, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(PlannedActionVisitor)

	rawReason := C.GoStringN(&action.reason[0], C.int(C.HYPHA_REASON_MAX_LENGTH))
	goReason, _, _ := strings.Cut(rawReason, "\x00")
	goAction := PlannedAction{
		ID:     C.GoString(action.id),
		Action: C.GoString(C.ControllerActionToCString(action.action)),
		Reason: goReason,
	}
	return C.bool(vis(uint64(idx), goAction))
}

func (plan *Plan) VisitPlannedActions(vis PlannedActionVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	C.VisitPlannedActions(
		plan.Handle,
		(C.VisitPlannedActionFn)(unsafe.Pointer(C.goVisitPlannedActions)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}
