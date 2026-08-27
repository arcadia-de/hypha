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

// coreResourceNamespace mirrors kCoreResourceNamespace in
// hypha/resource_namespace.h. Kept as a Go constant (rather than reading the
// C static const array via CGo) so the value is unambiguous; if the C
// constant ever changes, update this alongside it.
const coreResourceNamespace = "hypha"

// IsReservedResourceNamespace reports whether ns is reserved for
// orchestrator-internal use. Mirrors IsReservedResourceNamespace/
// IsDefaultResourceNamespace in hypha/resource_namespace.h: the default
// (empty) namespace is never reserved.
func IsReservedResourceNamespace(ns string) bool {
	return ns != "" && ns == coreResourceNamespace
}

// setResourceNamespace writes ns into a graph-resident Resource's namespace
// field in place, truncating to fit and zero-filling the remainder. Mirrors
// the C SetResourceNamespace inline helper in hypha/resource_namespace.h.
func setResourceNamespace(res *C.Resource, ns string) {
	buf := &res.info.ns
	for i := range buf {
		buf[i] = 0
	}

	max := len(buf) - 1
	if len(ns) > max {
		ns = ns[:max]
	}
	for i := 0; i < len(ns); i++ {
		buf[i] = C.char(ns[i])
	}
}
