package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <stdbool.h>

#include "hypha.h"
#include "hypha/annotation.h"
#include "hypha/history.h"
#include "hypha/label.h"
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
	ID          string
	Kind        string
	Name        string
	HashBefore  uint64
	HashAfter   uint64
	Reason      string
	RunID       string
	AppliedAt   int64
	Action      string
	Status      ControllerStatus
	Labels      []string
	Annotations []ResourceAnnotation
}

type HistoryRecordVisitor func(HistoryRecord) bool

//export goHistoryLogVisit
func goHistoryLogVisit(rec *C.HistoryRecord, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(HistoryRecordVisitor)

	labels := []string{}
	labelSize := uintptr(C.HYPHA_LABEL_MAX_SIZE)
	labelBase := uintptr(unsafe.Pointer(rec.labels))
	for i := range int(rec.labels_len) {
		ptr := (*C.char)(unsafe.Pointer(labelBase + uintptr(i)*labelSize))
		labels = append(labels, C.GoString(ptr))
	}

	annotations := []ResourceAnnotation{}
	annoSize := unsafe.Sizeof(C.Annotation{})
	annoBase := uintptr(unsafe.Pointer(rec.annotations))
	for i := range int(rec.annotations_len) {
		anno := (*C.Annotation)(unsafe.Pointer(annoBase + uintptr(i)*annoSize))
		annotations = append(annotations, ResourceAnnotation{
			Key:   C.GoString((*C.char)(unsafe.Pointer(&anno.key))),
			Value: C.GoString((*C.char)(unsafe.Pointer(&anno.value))),
		})
	}

	goRec := HistoryRecord{
		ID:         C.GoString(rec.id),
		Kind:       C.GoString(rec.kind),
		HashBefore: uint64(rec.hash_before),
		HashAfter:  uint64(rec.hash_after),
		Reason:     C.GoString((*C.char)(unsafe.Pointer(&rec.reason))),
		//TODO(@s0cks): RunID:       uint64(rec.run_id),
		AppliedAt:   int64(rec.applied_at),
		Action:      GetControllerActionName(ControllerAction(rec.action)),
		Status:      ControllerStatus(rec.status),
		Labels:      labels,
		Annotations: annotations,
	}
	return C.bool(vis(goRec))
}

func (orc *Orchestrator) HistoryReplay(vis HistoryRecordVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	C.HistoryLogReplay(
		C.GetOrcHistoryLog(orc.Handle),
		(C.HistoryLogVisitFn)(unsafe.Pointer(C.goHistoryLogVisit)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}
