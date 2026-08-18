package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include "hypha/label.h"
#include "hypha/orchestrator.h"
#include "hypha/resource_selector.h"
*/
import "C"

import (
	"unsafe"
)

// bool VisitAllNonMatchingResources(const ResourceGraph* rg, const ResourceSelector* rs, ResourceVisitorFn fn,
//                                   void* data);

type ResourceSelector struct {
	Handle *C.ResourceSelector
}

func (selector *ResourceSelector) IsValid() bool {
	return selector.Handle != nil
}

func NewIdFilter(kind string) ResourceSelector {
	cId := C.CString(kind)
	defer C.free(unsafe.Pointer(cId))
	handle := C.NewIdResourceSelector(cId)
	return ResourceSelector{
		Handle: handle,
	}
}

func NewKindResourceSelector(kind string) ResourceSelector {
	cKind := C.CString(kind)
	defer C.free(unsafe.Pointer(cKind))
	handle := C.NewKindResourceSelector(cKind)
	return ResourceSelector{
		Handle: handle,
	}
}

func NewIdsResourceSelector(ids []string) ResourceSelector {
	var filters []ResourceSelector
	for _, id := range ids {
		filters = append(filters, NewIdFilter(id))
	}
	return NewOrResourceSelector(filters)
}

func NewLabelResourceSelector(rhs string) ResourceSelector {
	var dummyLabel C.Label
	labelSize := int(unsafe.Sizeof(dummyLabel))

	cLabelMemory := C.malloc(C.size_t(labelSize))
	defer C.free(cLabelMemory)

	C.memset(cLabelMemory, 0, C.size_t(labelSize))
	rawByteSlice := unsafe.Slice((*byte)(cLabelMemory), labelSize)
	maxSafeLength := labelSize - 1
	if len(rhs) > maxSafeLength {
		rhs = rhs[:maxSafeLength]
	}
	copy(rawByteSlice, rhs)

	cgoExpectedPtr := (*[C.HYPHA_LABEL_MAX_SIZE]C.char)(cLabelMemory)
	handle := C.NewLabelResourceSelector(cgoExpectedPtr)
	return ResourceSelector{
		Handle: handle,
	}
}

func NewOrResourceSelector(selectors []ResourceSelector) ResourceSelector {
	numSelectors := len(selectors)
	cSelectors := C.malloc(C.size_t(numSelectors) * C.size_t(unsafe.Sizeof(uintptr(0))))
	defer C.free(cSelectors)

	cSelectorsSlice := (*[1 << 30]*C.ResourceSelector)(cSelectors)[:numSelectors:numSelectors]
	for i, s := range selectors {
		cSelectorsSlice[i] = s.Handle
	}

	handle := C.NewOrResourceSelector((**C.ResourceSelector)(cSelectors), C.uint64_t(numSelectors))
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
