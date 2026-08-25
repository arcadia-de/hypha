package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>

#include "hypha/resource.h"
*/
import "C"

import (
	"strings"
)

func GetResourceNamespace(res *C.Resource) string {
	rawValue := C.GoStringN(&res.info.ns[0], C.int(C.HYPHA_RESOURCE_NAMESPACE_MAX_SIZE))
	value, _, _ := strings.Cut(rawValue, "\x00")
	return value
}
