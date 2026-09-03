package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include "hypha/platform.h"
*/
import "C"

import (
	"github.com/google/go-jsonnet"
	"unsafe"

	lib "github.com/arcadia-de/hypha/jsonnet"
)

func HandleGetOS(args []any) (any, error) {
	cOS := C.GetOS()
	defer C.free(unsafe.Pointer(cOS))

	return C.GoString(cOS), nil
}

func HandleGetArch(args []any) (any, error) {
	cArch := C.GetArch()
	defer C.free(unsafe.Pointer(cArch))

	return C.GoString(cArch), nil
}

func HandleGetUsername(args []any) (any, error) {
	cUsername := C.GetUsername()
	defer C.free(unsafe.Pointer(cUsername))

	return C.GoString(cUsername), nil
}

func HandleGetHostname(args []any) (any, error) {
	cHostname := C.GetHostname()
	defer C.free(unsafe.Pointer(cHostname))

	return C.GoString(cHostname), nil
}

func HandleGetDistro(args []any) (any, error) {
	cDistro := C.GetDistro()
	defer C.free(unsafe.Pointer(cDistro))

	return C.GoString(cDistro), nil
}

func CreateJsonnetVM() *jsonnet.VM {
	vm := jsonnet.MakeVM()
	vm.Importer(&EmbedImporter{
		FS: lib.LibsonnetFiles,
	})

	// vm.ExtVar("env", "production")
	// vm.ExtCode("features", `{"enableBeta": true, "maxUsers": 100}`)

	natives := []jsonnet.NativeFunction{
		jsonnet.NativeFunction{
			Name: "getArch",
			Func: HandleGetArch,
		},
		jsonnet.NativeFunction{
			Name: "getOS",
			Func: HandleGetOS,
		},
		jsonnet.NativeFunction{
			Name: "getUsername",
			Func: HandleGetUsername,
		},
		jsonnet.NativeFunction{
			Name: "getHostname",
			Func: HandleGetHostname,
		},
		jsonnet.NativeFunction{
			Name: "getDistro",
			Func: HandleGetDistro,
		},
	}

	for _, native := range natives {
		vm.NativeFunction(&native)
	}

	return vm
}
