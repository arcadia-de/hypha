package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>

#include "hypha.h"
#include "hypha/resource.h"
#include "hypha/controller.h"

bool goVisitController(uint32_t idx, char* kind, Controller* ctrl, void* data);
*/
import "C"
import (
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type ControllerAction int

const (
	kNoAction ControllerAction = iota
	kCreateAction
	kUpdateAction
	kDestroyAction
	kFailedAction
)

func GetControllerActionName(action ControllerAction) string {
	switch action {
	case kNoAction:
		return "No Action"
	case kCreateAction:
		return "Create"
	case kUpdateAction:
		return "Update"
	case kDestroyAction:
		return "Destroy"
	case kFailedAction:
		return "Failed"
	default:
		return "Unknown"
	}
}

type Controller struct {
	Index  uint64
	Kind   string
	Handle *C.Controller
}

type ControllerVisitFn func(ctrl Controller) bool

//export goVisitController
func goVisitController(idx C.uint32_t, kind *C.char, ctrl *C.Controller, data *C.void) C.bool {
	handle := cgo.Handle(*(*uintptr)(unsafe.Pointer(data)))
	vis := handle.Value().(ControllerVisitFn)

	goController := Controller{
		Index:  uint64(idx),
		Kind:   C.GoString(kind),
		Handle: ctrl,
	}
	return C.bool(vis(goController))
}

func VisitControllers(vis ControllerVisitFn) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	cb := C.ControllerVisitFn(unsafe.Pointer(C.goVisitController))
	C.VisitAllControllers(
		cb,
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}

func GetAllControllerKinds() []string {
	var results []string
	VisitControllers(func(ctrl Controller) bool {
		results = append(results, ctrl.Kind)
		return true
	})
	return results
}
