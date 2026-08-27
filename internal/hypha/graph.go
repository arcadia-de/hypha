package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <string.h>
#include <xxhash.h>

#include "hypha.h"
#include "hypha/name.h"
#include "hypha/resource_id.h"
#include "hypha/resource_state.h"
#include "hypha/resource_graph.h"
#include "hypha/state.h"

bool goVisitResource(ResourceGraphIndex, Resource* res, void* data);
*/
import "C"

import (
	"encoding/json"
	"fmt"
	"runtime"
	"runtime/cgo"
	"strings"
	"unsafe"
)

// formatResourceID renders a raw ResourceId (uuid_t, 16 bytes) as a
// canonical 8-4-4-4-12 lowercase hex string, matching libuuid's uuid_unparse_lower.
func formatResourceID(id [16]C.uchar) string {
	var raw [16]byte
	for i := 0; i < 16; i++ {
		raw[i] = byte(id[i])
	}
	return fmt.Sprintf("%x-%x-%x-%x-%x", raw[0:4], raw[4:6], raw[6:8], raw[8:10], raw[10:16])
}

type ResourceGraph struct {
	Handle *C.ResourceGraph
}

type ResourceVisitor func(uint64, Resource) bool

//export goVisitResource
func goVisitResource(idx C.ResourceGraphIndex, res *C.Resource, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(ResourceVisitor)

	labels := []string{}
	count := int(res.info.labels_len)
	labelSize := uintptr(C.HYPHA_LABEL_MAX_SIZE)
	basePtr := uintptr(unsafe.Pointer(res.info.labels))

	for i := range count {
		offset := basePtr + (uintptr(i) * labelSize)
		ptr := (*C.char)(unsafe.Pointer(offset))
		rawLabel := C.GoStringN(ptr, C.int(C.HYPHA_LABEL_MAX_SIZE))
		goLabel, _, _ := strings.Cut(rawLabel, "\x00")
		labels = append(labels, goLabel)
	}

	annotations := []ResourceAnnotation{}
	num_annotations := int(res.info.annotations_len)
	annotationSize := unsafe.Sizeof(C.Annotation{})
	annotationsStart := uintptr(unsafe.Pointer(res.info.annotations))

	for i := range num_annotations {
		offset := annotationsStart + (uintptr(i) * annotationSize)
		namePtr := offset
		valuePtr := offset + uintptr(C.HYPHA_ANNOTATION_KEY_SIZE)

		annotations = append(annotations, ResourceAnnotation{
			Key:   C.GoStringN((*C.char)(unsafe.Pointer(namePtr)), C.HYPHA_ANNOTATION_KEY_SIZE),
			Value: C.GoStringN((*C.char)(unsafe.Pointer(valuePtr)), C.HYPHA_ANNOTATION_VALUE_SIZE),
		})
	}

	rawReason := C.GoStringN(&res.status.reason[0], C.int(C.HYPHA_REASON_MAX_LENGTH))
	goReason, _, _ := strings.Cut(rawReason, "\x00")

	ns := GetResourceNamespace(res)

	var spec any
	goSpecStr := C.GoString(res.spec.raw)
	if goSpecStr != "" {
		if err := json.Unmarshal([]byte(goSpecStr), &spec); err != nil {
			fmt.Printf("failed to unmarshal spec field `%s`: %v", goSpecStr, err)
			spec = goSpecStr
		}
	} else {
		spec = map[string]any{}
	}

	kindStr := C.GoString(C.FindResourceKindName(res.kind))

	goResource := Resource{
		ID:    formatResourceID(res.id),
		Kind:  kindStr,
		State: C.GoString(C.ResourceStateCStr(res.state)),
		Metadata: ResourceMetadata{
			Name:        C.GoString(res.info.name),
			Namespace:   ns,
			Labels:      labels,
			Annotations: annotations,
		},
		Spec: spec,
		Status: ResourceStatus{
			State:  ResourceState(res.status.state),
			Action: ControllerAction(res.status.action),
			Reason: goReason,

			//TODO(@s0cks):
			// struct timespec timestamp;
		},
	}

	return C.bool(vis(uint64(idx), goResource))
}

func (rg *ResourceGraph) VisitAllResources(vis ResourceVisitor) bool {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	success := C.VisitAllResources(
		rg.Handle,
		(C.ResourceVisitorFn)(unsafe.Pointer(C.goVisitResource)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
	return bool(success)
}

func (rg *ResourceGraph) VisitAllMatchingResources(rs ResourceSelector, vis ResourceVisitor) bool {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	success := C.VisitAllMatchingResources(
		rg.Handle,
		rs.Handle,
		(C.ResourceVisitorFn)(unsafe.Pointer(C.goVisitResource)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
	return bool(success)
}

func (rg *ResourceGraph) ListResources() []Resource {
	var records []Resource
	rg.VisitAllResources(func(idx uint64, rec Resource) bool {
		records = append(records, rec)
		return true
	})
	return records
}

func (rg *ResourceGraph) ListResourcesWithSelector(selector ResourceSelector) []Resource {
	var records []Resource

	if selector.Handle != nil {
		rg.VisitAllMatchingResources(selector, func(idx uint64, rec Resource) bool {
			records = append(records, rec)
			return true
		})
	} else {
		rg.VisitAllResources(func(idx uint64, rec Resource) bool {
			records = append(records, rec)
			return true
		})
	}

	return records
}

func (rg *ResourceGraph) ListResourcesWithOptionalSelector(selector ResourceSelector) []Resource {
	if selector.IsValid() {
		return rg.ListResourcesWithSelector(selector)
	}

	return rg.ListResources()
}

func (rg *ResourceGraph) IsEmpty() bool {
	return bool(C.IsResourceGraphEmpty(rg.Handle))
}

// AddResource allocates a new Resource directly inside the underlying C
// resource graph and populates every field on it in place.
//
// This replaces the old flow where Go built a standalone C.Resource and
// passed it to OrchestratorAddResource, which then allocated a *second*
// Resource in the graph and deep-copied every field (name, labels,
// annotations, depends_on) across. There's now exactly one allocation of
// each field, written straight into the resource that lives in the graph.
//
// Callers are expected to have already validated kind and namespace (see
// Orchestrator.AddResource) — this function assumes both are valid and
// focuses purely on construction.
func (rg *ResourceGraph) AddResource(
	store *C.StateStore,
	kind ResourceKind,
	name string,
	namespace string,
	labels []string,
	annotations []ResourceAnnotation,
	dependsOn []string,
	rawSpec string,
) error {
	newRes := C.AllocNewResouceInGraph(rg.Handle)
	if newRes == nil {
		return fmt.Errorf("failed to allocate resource in graph")
	}

	cKind := C.ResourceKind(kind)

	resolvedName := name
	if resolvedName == "" {
		// TODO(@s0cks): optimize name generation
		var nameBuf C.Name
		C.GenerateDefaultResourceName(cKind, &nameBuf)
		rawName := C.GoStringN(&nameBuf[0], C.int(len(nameBuf)))
		resolvedName, _, _ = strings.Cut(rawName, "\x00")
	}
	newRes.info.name = C.CString(resolvedName)

	setResourceNamespace(newRes, namespace)
	newRes.id = resolveOrGenerateResourceId(store, cKind, resolvedName)
	newRes.kind = cKind

	if rawSpec != "" {
		cRawSpec := C.CString(rawSpec)
		newRes.spec.raw = cRawSpec
		newRes.spec.doc = nil
		newRes.spec.hash = C.uint64_t(C.XXH3_64bits(unsafe.Pointer(cRawSpec), C.size_t(len(rawSpec))))
	}

	setResourceLabels(newRes, labels)
	setResourceAnnotations(newRes, annotations)

	newRes.state = C.kResourcePending
	setResourceDependsOn(newRes, dependsOn)

	return nil
}

// resolveOrGenerateResourceId mirrors ResolveOrGenerateResourceId from the
// old orchestrator_add.c: reuse the id already on record for (kind, name) in
// the state store, the k8s-uid model of stable identity across runs
// (see StateStoreFindIdByName), falling back to a freshly generated id if
// there's no name to key off of, no prior record, or the recorded id string
// doesn't parse.
func resolveOrGenerateResourceId(store *C.StateStore, kind C.ResourceKind, name string) C.ResourceId {
	var id C.ResourceId

	if store != nil && name != "" && kind != C.ResourceKind(InvalidResourceKind) {
		if kindName := C.FindResourceKindName(kind); kindName != nil {
			cName := C.CString(name)
			existingID := C.StateStoreFindIdByName(store, kindName, cName)
			C.free(unsafe.Pointer(cName))

			if existingID != nil {
				parsed := C.uuid_parse(existingID, &id[0]) == 0
				C.free(unsafe.Pointer(existingID))
				if parsed {
					return id
				}
				// fall through: recorded id string didn't parse, generate a fresh one
			}
		}
	}

	C.GenerateResourceId(&id)
	return id
}

// setResourceLabels mallocs a Label array sized for labels and writes it
// directly onto res.info, truncating any label that doesn't fit.
func setResourceLabels(res *C.Resource, labels []string) {
	n := len(labels)
	if n == 0 {
		res.info.labels = nil
		res.info.labels_len = 0
		res.info.labels_cap = 0
		return
	}

	var dummy C.Label
	labelSize := int(unsafe.Sizeof(dummy))
	buf := C.malloc(C.size_t(n) * C.size_t(labelSize))
	C.memset(buf, 0, C.size_t(n)*C.size_t(labelSize))
	raw := unsafe.Slice((*byte)(buf), n*labelSize)

	maxLen := labelSize - 1
	for i, label := range labels {
		if len(label) > maxLen {
			label = label[:maxLen]
		}
		copy(raw[i*labelSize:i*labelSize+len(label)], label)
	}

	res.info.labels = (*C.Label)(buf)
	res.info.labels_len = C.size_t(n)
	res.info.labels_cap = C.size_t(n)
}

// setResourceAnnotations mallocs an Annotation array sized for annotations
// and writes it directly onto res.info, truncating any key/value that
// doesn't fit its fixed-size field.
func setResourceAnnotations(res *C.Resource, annotations []ResourceAnnotation) {
	n := len(annotations)
	if n == 0 {
		res.info.annotations = nil
		res.info.annotations_len = 0
		res.info.annotations_cap = 0
		return
	}

	var dummy C.Annotation
	annoSize := int(unsafe.Sizeof(dummy))
	buf := C.malloc(C.size_t(n) * C.size_t(annoSize))
	C.memset(buf, 0, C.size_t(n)*C.size_t(annoSize))
	raw := unsafe.Slice((*byte)(buf), n*annoSize)

	keySize := int(C.HYPHA_ANNOTATION_KEY_SIZE)
	valSize := int(C.HYPHA_ANNOTATION_VALUE_SIZE)
	maxKeyLen := keySize - 1
	maxValLen := valSize - 1

	for i, anno := range annotations {
		offset := i * annoSize

		key, val := anno.Key, anno.Value
		if len(key) > maxKeyLen {
			key = key[:maxKeyLen]
		}
		if len(val) > maxValLen {
			val = val[:maxValLen]
		}

		copy(raw[offset:offset+len(key)], key)
		copy(raw[offset+keySize:offset+keySize+len(val)], val)
	}

	res.info.annotations = (*C.Annotation)(buf)
	res.info.annotations_len = C.size_t(n)
	res.info.annotations_cap = C.size_t(n)
}

// setResourceDependsOn mallocs a char* array sized for dependsOn and writes
// it directly onto res.
func setResourceDependsOn(res *C.Resource, dependsOn []string) {
	n := len(dependsOn)
	res.num_depends_on = C.size_t(n)
	if n == 0 {
		res.depends_on = nil
		return
	}

	buf := C.malloc(C.size_t(n) * C.size_t(unsafe.Sizeof(uintptr(0))))
	slice := (*[1 << 30]*C.char)(buf)[:n:n]
	for i, dep := range dependsOn {
		slice[i] = C.CString(dep)
	}
	res.depends_on = (**C.char)(buf)
}
