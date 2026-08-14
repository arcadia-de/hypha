package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <stdbool.h>
#include "hypha.h"
#include "hypha/history.h"
#include "hypha/resource.h"
#include "hypha/controller.h"
#include "hypha/orchestrator.h"

bool goHistoryLogVisit(HistoryRecord* rec, void* data);
*/
import "C"

import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type HistoryRecord struct {
	ID         string
	Kind       string
	HashBefore string
	HashAfter  string
	Reason     string
	RunID      uint64
	AppliedAt  int64
	Action     string
	Status     string
}

type HistoryRecordVisitor func(HistoryRecord) bool

//export goHistoryLogVisit
func goHistoryLogVisit(rec *C.HistoryRecord, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(HistoryRecordVisitor)

	goRec := HistoryRecord{
		ID:         C.GoString(rec.id),
		Kind:       C.GoString(rec.kind),
		HashBefore: C.GoString(rec.hash_before),
		HashAfter:  C.GoString(rec.hash_after),
		Reason:     C.GoString(rec.reason),
		RunID:      uint64(rec.run_id),
		AppliedAt:  int64(rec.applied_at),
		Action:     GetControllerActionName(ControllerAction(rec.action)),
		Status:     GetControllerStatusName(ControllerStatus(rec.status)),
	}
	return C.bool(vis(goRec))
}

func (orc *Orchestrator) HistoryReplay(vis HistoryRecordVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	C.HistoryLogReplay(
		C.OrchestratorGetHistoryLog(orc.Handle),
		(C.HistoryLogVisitFn)(unsafe.Pointer(C.goHistoryLogVisit)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}
