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
	"fmt"
	"strings"
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

type ControllerStatus int

const (
	kOkStatus ControllerStatus = iota
	kNoOpStatus
	kInvalidSpecStatus
	kNotFoundStatus
	kConflictStatus
	kUnsupportedStatus
	kTransientErrorStatus
	kPermanentErrorStatus
	kInternalErrorStatus
)

func GetControllerStatusName(status ControllerStatus) string {
	switch status {
	case kOkStatus:
		return "Ok"
	case kNoOpStatus:
		return "NoOp"
	case kInvalidSpecStatus:
		return "Invalid Spec"
	case kNotFoundStatus:
		return "Not Found"
	case kConflictStatus:
		return "Conflict"
	case kUnsupportedStatus:
		return "Unsupported"
	case kTransientErrorStatus:
		return "Transient Error"
	case kPermanentErrorStatus:
		return "Permanent Error"
	case kInternalErrorStatus:
		return "Internal Error"
	default:
		return "Unknown"
	}
}

//export goVisitController
func goVisitController(idx C.uint32_t, kind *C.char, ctrl *C.Controller, data *C.void) C.bool {
	rawPtr := unsafe.Pointer(data)
	if rawPtr == nil {
		return C.bool(false)
	}

	slicePtr := (*[]string)(rawPtr)
	goKind := C.GoString(kind)
	*slicePtr = append(*slicePtr, strings.ToLower(goKind))
	return C.bool(true)
}

func GetAllControllerKinds() []string {
	cb := C.ControllerVisitFn(C.goVisitController)

	var results []string
	ctxPointer := unsafe.Pointer(&results)
	success := C.VisitAllControllers(cb, ctxPointer)
	if !bool(success) {
		fmt.Println("C visitor failed")
		return []string{}
	}

	return results
}
