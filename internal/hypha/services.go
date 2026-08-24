package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>

#include "hypha/service_manager.h"

bool goVisitServiceManager(uint64_t, ServiceManager*, void*);
*/
import "C"
import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type ServiceManager struct {
	Handle *C.ServiceManager
}

func (sm *ServiceManager) GetName() string {
	if sm.Handle == nil {
		return ""
	}

	return C.GoString(C.GetServiceManagerName(sm.Handle))
}

func GetTotalNumberOfServiceManagers() uint64 {
	return uint64(C.GetTotalNumberOfServiceManagers())
}

func GetServiceManagerAt(idx uint64) ServiceManager {
	handle := C.GetServiceManagerAt(C.size_t(idx))
	return ServiceManager{
		Handle: handle,
	}
}

func FindServiceManager(name string) ServiceManager {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	handle := C.FindServiceManager(cName)
	return ServiceManager{
		Handle: handle,
	}
}

type ServiceManagerVisitor func(sm ServiceManager) bool

//export goVisitServiceManager
func goVisitServiceManager(idx C.uint64_t, sm *C.ServiceManager, data *C.void) C.bool {
	handle := cgo.Handle(*(*uintptr)(unsafe.Pointer(data)))
	vis := handle.Value().(ServiceManagerVisitor)

	goServiceManager := ServiceManager{
		Handle: sm,
	}
	return C.bool(vis(goServiceManager))
}

func VisitAllServiceManagers(vis ServiceManagerVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	cb := C.VisitServiceManagerFn(unsafe.Pointer(C.goVisitServiceManager))
	C.VisitAllServiceManagers(
		cb,
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}

func GetAllServiceManagers() []ServiceManager {
	managers := []ServiceManager{}
	VisitAllServiceManagers(func(sm ServiceManager) bool {
		managers = append(managers, sm)
		return true
	})
	return managers
}

func GetAllServiceManagerNames() []string {
	names := []string{}
	VisitAllServiceManagers(func(sm ServiceManager) bool {
		names = append(names, sm.GetName())
		return true
	})
	return names
}
