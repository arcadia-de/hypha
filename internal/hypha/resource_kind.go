package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>

#include "hypha/resource_kind.h"

bool goVisitResourceKind(ResourceKindInfo*, void*);
*/
import "C"
import (
	"fmt"
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type ResourceKind int64

const (
	InvalidResourceKind ResourceKind = -1
)

type ResourceKindInfo struct {
	Kind ResourceKind
	Name string
}

func GetTotalNumberOfResourceKinds() uint64 {
	return uint64(C.GetTotalNumberOfResourceKinds())
}

func FindResourceKind(name string) ResourceKind {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))
	return ResourceKind(C.FindResourceKind(cName))
}

func GetResourceKindInfo(kind ResourceKind) (*ResourceKindInfo, error) {
	cInfo := C.GetResourceKindInfo(C.ResourceKind(kind))
	if cInfo == nil {
		return nil, fmt.Errorf("failed to get ResourceKind for %d", kind)
	}

	return &ResourceKindInfo{
		Kind: ResourceKind(cInfo.kind),
		Name: C.GoString(cInfo.name),
	}, nil
}

func FindResourceKindInfo(name string) (*ResourceKindInfo, error) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))
	cInfo := C.FindResourceKindInfo(cName)
	if cInfo == nil {
		return nil, fmt.Errorf("failed to find ResourceKind for: %s", name)
	}

	return &ResourceKindInfo{
		Kind: ResourceKind(cInfo.kind),
		Name: C.GoString(cInfo.name),
	}, nil
}

type ResourceKindVisitor func(info ResourceKindInfo) bool

//export goVisitResourceKind
func goVisitResourceKind(info *C.ResourceKindInfo, data *C.void) C.bool {
	handle := cgo.Handle(*(*uintptr)(unsafe.Pointer(data)))
	vis := handle.Value().(ResourceKindVisitor)

	goInfo := ResourceKindInfo{
		Kind: ResourceKind(info.kind),
		Name: C.GoString(info.name),
	}
	return C.bool(vis(goInfo))
}

func VisitAllResourceKinds(vis ResourceKindVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	cb := C.VisitResourceKindFn(unsafe.Pointer(C.goVisitResourceKind))
	C.VisitAllResourceKinds(
		cb,
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}

func GetAllResourceKindInfos() []ResourceKindInfo {
	infos := []ResourceKindInfo{}
	VisitAllResourceKinds(func(info ResourceKindInfo) bool {
		infos = append(infos, info)
		return true
	})
	return infos
}

func GetAllResourceKindNames() []string {
	names := []string{}
	VisitAllResourceKinds(func(info ResourceKindInfo) bool {
		names = append(names, info.Name)
		return true
	})
	return names
}
