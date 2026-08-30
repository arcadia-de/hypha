package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/action_log.h"

bool goVisitAppliedActions(uint64_t, AppliedAction*, void*);
*/
import "C"

import (
	"runtime"
	"runtime/cgo"
	"strings"
	"unsafe"
)

type AppliedAction struct {
	Action uint32
	Name   string
	Kind   string
	Reason string
}

type AppliedActionVisitor func(idx uint64, act AppliedAction) bool

type AppliedActionLog struct {
	Handle *C.AppliedActionLog
}

//export goVisitAppliedActions
func goVisitAppliedActions(idx C.uint64_t, act *C.AppliedAction, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(AppliedActionVisitor)

	goName := C.GoString(act.resource.info.name)

	goKind := C.GoString(C.FindResourceKindName(act.resource.kind))

	rawReason := C.GoStringN(&act.reason[0], C.int(C.HYPHA_REASON_MAX_LENGTH))
	goReason, _, _ := strings.Cut(rawReason, "\x00")
	goAction := AppliedAction{
		Action: uint32(act.action),
		Name:   goName,
		Kind:   goKind,
		Reason: goReason,
	}
	return C.bool(vis(uint64(idx), goAction))
}

func (log *AppliedActionLog) VisitAppliedActions(vis AppliedActionVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	C.VisitAllAppliedActions(
		log.Handle,
		(C.VisitAppliedActionFn)(unsafe.Pointer(C.goVisitAppliedActions)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}
