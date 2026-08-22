package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include "hypha.h"
#include "hypha/resource_state.h"
#include "hypha/resource_graph.h"

bool goVisitResource(ResourceGraphIndex, Resource* res, void* data);
*/
import "C"

import (
	"fmt"
	"runtime"
	"runtime/cgo"
	"unsafe"
)

// formatResourceID renders a raw ResourceId (uuid_t, 16 bytes) as a
// canonical 8-4-4-4-12 lowercase hex string, matching libuuid's uuid_unparse_lower.
func formatResourceID(id [16]C.uchar) string {
	var raw [16]byte
	for i := 0; i < 16; i++ {
		raw[i] = byte(id[i])
	}
	return fmt.Sprintf("%x-%x-%x-%x-%x", raw[0:4], raw[4:6], raw[6:8], raw[8:10], raw[10:16])
}

type ResourceGraph struct {
	Handle *C.ResourceGraph
}

type ResourceVisitor func(uint64, Resource) bool

//export goVisitResource
func goVisitResource(idx C.ResourceGraphIndex, res *C.Resource, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(ResourceVisitor)

	labels := []string{}
	count := int(res.info.labels_len)
	labelSize := uintptr(C.HYPHA_LABEL_MAX_SIZE)
	basePtr := uintptr(unsafe.Pointer(res.info.labels))

	for i := range count {
		offset := basePtr + (uintptr(i) * labelSize)
		ptr := (*C.char)(unsafe.Pointer(offset))
		labels = append(labels, C.GoStringN(ptr, C.int(labelSize)))
	}

	goResource := Resource{
		ID:    formatResourceID(res.id),
		Kind:  C.GoString(res.kind),
		State: C.GoString(C.ResourceStateCStr(res.state)),
		Metadata: ResourceMetadata{
			Name:   C.GoString(res.info.name),
			Labels: labels,
		},
	}

	return C.bool(vis(uint64(idx), goResource))
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
	rg.VisitAllResources(func(idx uint64, rec Resource) bool {
		records = append(records, rec)
		return true
	})
	return records
}

func (rg *ResourceGraph) ListResourcesWithSelector(selector ResourceSelector) []Resource {
	var records []Resource
	rg.VisitAllMatchingResources(selector, func(idx uint64, rec Resource) bool {
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

func (rg *ResourceGraph) IsEmpty() bool {
	return bool(C.IsResourceGraphEmpty(rg.Handle))
}
