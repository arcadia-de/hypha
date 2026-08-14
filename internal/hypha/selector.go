package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include "hypha/orchestrator.h"
#include "hypha/resource_selector.h"

bool goVisitResource(Resource* res, void* data);
*/
import "C"

import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type Resource struct {
	ID   string
	Kind string
}

type ResourceVisitor func(Resource) bool

//export goVisitResource
func goVisitResource(res *C.Resource, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(ResourceVisitor)

	goResource := Resource{
		ID:   C.GoString(res.id),
		Kind: C.GoString(res.kind),
	}
	return C.bool(vis(goResource))
}

// bool VisitAllResources(const ResourceGraph* rg, ResourceVisitorFn fn, void* data);
func (orc *Orchestrator) VisitAllResources(vis ResourceVisitor) bool {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	success := C.VisitAllResources(
		C.OrchestratorGetResourceGraph(orc.Handle),
		(C.ResourceVisitorFn)(unsafe.Pointer(C.goVisitResource)),
		unsafe.Pointer(&handle),
	)
	runtime.KeepAlive(handle)
	return bool(success)
}

// bool VisitAllMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn, void* data);
func (orc *Orchestrator) VisitAllMatchingResources(rs ResourceSelector, vis ResourceVisitor) bool {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	success := C.VisitAllMatchingResources(
		C.OrchestratorGetResourceGraph(orc.Handle),
		rs.Handle,
		(C.ResourceVisitorFn)(unsafe.Pointer(C.goVisitResource)),
		unsafe.Pointer(&handle),
	)
	runtime.KeepAlive(handle)
	return bool(success)
}

// bool VisitAllNonMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn,
//                                   void* data);

type ResourceSelector struct {
	Handle *C.ResourceSelector
}

func NewKindResourceSelector(kind string) ResourceSelector {
	cKind := C.CString(kind)
	defer C.free(unsafe.Pointer(cKind))
	handle := C.NewKindResourceSelector(cKind)
	return ResourceSelector{
		Handle: handle,
	}
}

func NewLabelResourceSelector(rhs string) ResourceSelector {
	cLabel := C.CString(rhs)
	defer C.free(unsafe.Pointer(cLabel))
	handle := C.NewLabelResourceSelector(cLabel)
	return ResourceSelector{
		Handle: handle,
	}
}

func NewAndResourceSelector(selectors []ResourceSelector) ResourceSelector {
	numSelectors := len(selectors)
	cSelectors := C.malloc(C.size_t(numSelectors) * C.size_t(unsafe.Sizeof(uintptr(0))))
	defer C.free(cSelectors)

	cSelectorsSlice := (*[1 << 30]*C.ResourceSelector)(cSelectors)[:numSelectors:numSelectors]
	for i, s := range selectors {
		cSelectorsSlice[i] = s.Handle
	}

	handle := C.NewAndResourceSelector((**C.ResourceSelector)(cSelectors), C.uint64_t(numSelectors))
	return ResourceSelector{
		Handle: handle,
	}
}

func (rs *ResourceSelector) Close() {
	C.FreeResourceSelector(rs.Handle)
}

// ResourceSelector* NewResourceSelector(ResourceSelectorFn fn, void* data, void (*free_data)(void*));
// ResourceSelector* NewOrResourceSelector(ResourceSelector** selectors, const uint64_t num_selectors);
// ResourceSelector* NewAnnotationResourceSelector(const ResourceAnnotation* rhs);
// ResourceSelector* NewAnnotationKeyResourceSelector(const char* rhs);
// ResourceSelector* NewAnnotationValueResourceSelector(const char* rhs);
// bool ResourceSelectorMatch(const ResourceSelector* rs, const Resource* res);
// void FreeResourceSelector(ResourceSelector* rs);
