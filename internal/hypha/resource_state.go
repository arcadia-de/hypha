package hypha

/*
#cgo pkg-config: hypha-uninstalled

#include <stdlib.h>
#include "hypha/resource.h"
*/
import "C"

type ResourceState int

const (
	ResourcePending    ResourceState = C.kResourcePending
	ResourceProcessing ResourceState = C.kResourceProcessing
	ResourceReady      ResourceState = C.kResourceReady
	ResourceFailed     ResourceState = C.kResourceFailed
	ResourceUnknown    ResourceState = C.kResourceUnknown
)

func (state ResourceState) String() string {
	return C.GoString(C.ResourceStateCStr(C.ResourceState(state)))
}
