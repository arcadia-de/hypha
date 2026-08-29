package hypha

/*
#cgo pkg-config: hypha-uninstalled

#include <uuid/uuid.h>
#include "hypha/run_info.h"
*/
import "C"
import (
	"unsafe"
)

// "fmt"
// "runtime"
// "runtime/cgo"

type RunMode int

const (
	RunObserve      RunMode = C.kOrchestratorObserveMode
	RunPlanMode     RunMode = C.kOrchestratorPlanMode
	RunDiffMode     RunMode = C.kOrchestratorDiffMode
	RunValidateMode RunMode = C.kOrchestratorValidateMode
	RunApplyMode    RunMode = C.kOrchestratorApplyMode
	RunDestroyMode  RunMode = C.kOrchestratorDestroyMode
)

type RunInfo struct {
	Mode   RunMode
	Reason string
}

func (info *RunInfo) ToC() *C.RunInfo {
	ptr := (*C.RunInfo)(C.malloc(C.sizeof_RunInfo))
	C.memset(unsafe.Pointer(ptr), 0, C.sizeof_RunInfo)
	C.uuid_generate_random((*C.uchar)(&ptr.id[0]))

	goReason := info.Reason
	cReason := C.CString(goReason)
	defer C.free(unsafe.Pointer(cReason))

	C.memcpy(unsafe.Pointer(&ptr.reason[0]), unsafe.Pointer(cReason), min(C.strlen(cReason), C.HYPHA_REASON_MAX_LENGTH))

	ptr.mode = (C.OrchestratorRunMode)(info.Mode)

	return ptr
}
