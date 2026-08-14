package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include "hypha/package_manager.h"
*/
import "C"

import (
	"fmt"
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
		Handle: handle,
	}, nil
}

func (mgr *PackageManager) Install(pkg string) (PackageStatus, error) {
	cPkg := C.CString(pkg)
	defer C.free(unsafe.Pointer(cPkg))
	status := C.PackageManagerInstall(mgr.Handle, cPkg)
	return PackageStatus(status), nil
}
