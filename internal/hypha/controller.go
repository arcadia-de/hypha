package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include "hypha.h"
#include "hypha/resource.h"
#include "hypha/controller.h"

bool goVisitController(uint32_t idx, char* kind, Controller* ctrl, void* data);
bool goVisitControllerAppend(uint32_t idx, char* kind, Controller* ctrl, void* data);
*/
import "C"
import (
	"fmt"
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

//export goVisitControllerAppend
func goVisitControllerAppend(idx C.uint32_t, kind *C.char, ctrl *C.Controller, data *C.void) C.bool {
	rawPtr := unsafe.Pointer(data)
	if rawPtr == nil {
		return C.bool(false)
	}

	slicePtr := (*[]string)(rawPtr)
	goKind := C.GoString(kind)
	*slicePtr = append(*slicePtr, goKind)
	return C.bool(true)
}

func GetAllControllerKinds() []string {
	cb := C.ControllerVisitFn(C.goVisitControllerAppend)

	var results []string
	ctxPointer := unsafe.Pointer(&results)
	success := C.VisitAllControllers(cb, ctxPointer)
	if !bool(success) {
		fmt.Println("C visitor failed")
		return []string{}
	}

	return results
}
