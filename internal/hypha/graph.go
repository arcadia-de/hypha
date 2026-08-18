package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include "hypha.h"
#include "hypha/resource_state.h"
#include "hypha/resource_graph.h"

bool goVisitResource(Resource* res, void* data);
*/
import "C"

import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type ResourceGraph struct {
	Handle *C.ResourceGraph
}

type ResourceVisitor func(Resource) bool

//export goVisitResource
func goVisitResource(res *C.Resource, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(ResourceVisitor)

	labels := []string{}
	count := int(res.info.labels_len)
	labelSize := uintptr(C.HYPHA_LABEL_MAX_SIZE)
	basePtr := uintptr(unsafe.Pointer(res.info.labels))

	for i := 0; i < count; i++ {
		offset := basePtr + (uintptr(i) * labelSize)
		ptr := (*C.char)(unsafe.Pointer(offset))
		labels = append(labels, C.GoStringN(ptr, C.int(labelSize)))
	}

	goResource := Resource{
		ID:    C.GoString(res.id),
		Kind:  C.GoString(res.kind),
		State: C.GoString(C.ResourceStateCStr(res.state)),
		Metadata: ResourceMetadata{
			Labels: labels,
		},
	}

	return C.bool(vis(goResource))
}

func (rg *ResourceGraph) VisitAllResources(vis ResourceVisitor) bool {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	success := C.VisitAllResources(
		rg.Handle,
		(C.ResourceVisitorFn)(unsafe.Pointer(C.goVisitResource)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
	return bool(success)
}

func (rg *ResourceGraph) VisitAllMatchingResources(rs ResourceSelector, vis ResourceVisitor) bool {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	success := C.VisitAllMatchingResources(
		rg.Handle,
		rs.Handle,
		(C.ResourceVisitorFn)(unsafe.Pointer(C.goVisitResource)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
	return bool(success)
}

func (rg *ResourceGraph) ListResources() []Resource {
	var records []Resource
	rg.VisitAllResources(func(rec Resource) bool {
		records = append(records, rec)
		return true
	})
	return records
}

func (rg *ResourceGraph) ListResourcesWithSelector(selector ResourceSelector) []Resource {
	var records []Resource
	rg.VisitAllMatchingResources(selector, func(rec Resource) bool {
		records = append(records, rec)
		return true
	})
	return records
}

func (rg *ResourceGraph) ListResourcesWithOptionalSelector(selector ResourceSelector) []Resource {
	if selector.IsValid() {
		return rg.ListResourcesWithSelector(selector)
	}

	return rg.ListResources()
}
