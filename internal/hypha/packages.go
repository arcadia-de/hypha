package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <stdint.h>
#include "hypha/package_manager.h"

bool goVisitPackageManager(uint64_t, PackageManager*, void*);
*/
import "C"

import (
	"fmt"
	"runtime"
	"runtime/cgo"
	"unsafe"
)

type PackageStatus int

const (
	kPackageInstalled PackageStatus = iota
	kPackageSkipped
	kPackageUninstalled
	kPackageError
)

type PackageManager struct {
	Name   string
	Handle *C.PackageManager
}

func GetPackageManager(name string) (PackageManager, error) {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	handle := C.FindPackageManager(cName)
	if handle == nil {
		return PackageManager{}, fmt.Errorf("cannot find package manager for: %s", name)
	}

	return PackageManager{
		Name:   name,
		Handle: handle,
	}, nil
}

func (mgr *PackageManager) Install(pkg string) (PackageStatus, error) {
	cPkg := C.CString(pkg)
	defer C.free(unsafe.Pointer(cPkg))
	status := C.PackageManagerInstall(mgr.Handle, cPkg)
	return PackageStatus(status), nil
}

type PackageManagerVisitor func(idx uint64, pm PackageManager) bool

//export goVisitPackageManager
func goVisitPackageManager(idx C.uint64_t, pm *C.PackageManager, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(PackageManagerVisitor)

	goPm := PackageManager{
		Handle: pm,
		Name:   C.GoString(C.GetPackageManagerName(pm)),
	}
	return C.bool(vis(uint64(idx), goPm))
}

func VisitPackageManagers(vis PackageManagerVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	C.VisitAllPackageManagers(
		(C.PackageManagerVisitFn)(unsafe.Pointer(C.goVisitPackageManager)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}
