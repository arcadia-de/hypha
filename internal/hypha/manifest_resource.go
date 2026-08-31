package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <time.h>
#include <xxhash.h>

#include "hypha/orchestrator.h"
#include "hypha/resource_id.h"
#include "hypha/resource_kind.h"
#include "hypha/state.h"
*/
import "C"

import (
	"encoding/base64"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"
	"unsafe"
)

// manifestResourceKind mirrors kManifestResourceKindName in hypha/hypha.c. Kept as a Go constant
// (rather than reading the C #define, which cgo can't see) for the same reason
// coreResourceNamespace is: the value needs to be unambiguous on this side, and if the C
// constant ever changes, update this alongside it.
const manifestResourceKind = "Manifest"

// hyphaHashAnnotationKey mirrors the `hypha/hash` system annotation used elsewhere for the
// per-controller fast-path hash signal (see the Kahn scheduler / ControllerPlan). Manifest
// resources reuse the same annotation for the same reason: it's how "has this changed since
// last time" is already expressed on any resource, and Manifest resources fit that shape too
// (their "content" is the underlying discovered file/raw manifest bytes).
const hyphaHashAnnotationKey = "hypha/hash"

// ManifestResourceSpec is the .spec payload for a Manifest pseudo-resource: it records enough
// provenance to answer "what did this manifest look like when it was discovered, and what came
// out of it" via `hypha describe manifest <name>` without needing to re-read the file or
// re-run discovery.
type ManifestResourceSpec struct {
	// Source is the DiscoveredManifestKind that produced this entry: "Path", "RawJson",
	// "RawYaml", or "RawJsonnet".
	Source string `json:"source"`

	// Path is set only for Source == "Path": the on-disk manifest path as discovery reported
	// it, including any "#fragment" suffix. Empty for raw manifests, which have no path.
	Path string `json:"path,omitempty"`

	// Fragment is the portion of Path after "#", when present -- mirrors the
	// <filename>.jsonnet#<fragment> convention used to select a sub-document out of a single
	// discovered file. Empty when Path has no fragment, or for raw manifests.
	Fragment string `json:"fragment,omitempty"`

	// ContentBase64 is the manifest's raw bytes (file contents for Path, the literal value for
	// Raw*), base64-encoded so arbitrary manifest content round-trips safely through the JSON
	// spec document.
	ContentBase64 string `json:"contentBase64,omitempty"`

	// Hash is the hex xxh3 digest of the *un*-encoded content, i.e. XXH3_64bits over the same
	// bytes ContentBase64 encodes. Also mirrored onto the hypha/hash annotation.
	Hash string `json:"hash"`

	// ResourceCount is how many ResourceSpecs this manifest expanded into.
	ResourceCount int `json:"resourceCount"`

	// DiscoveredAt is when this manifest was processed, as unix seconds. Distinct from the
	// StateEntry's applied_at (only written on RunApplyMode): this is set on every run
	// regardless of persistence, so a read-only `describe` still shows when discovery last saw
	// this manifest, not just when it was last applied.
	DiscoveredAt int64 `json:"discoveredAt"`

	// ReadError records why ContentBase64/Hash may be empty: e.g. a Path manifest that no
	// longer exists or isn't readable. The Manifest resource is still created in this case, so
	// a broken discovered manifest is visible via `list`/`describe` rather than silently
	// vanishing.
	ReadError string `json:"readError,omitempty"`
}

// xxh3HexOf returns the lowercase hex xxh3 digest of data, matching the hash algorithm used
// elsewhere in the codebase (hypha/hash annotation, resource spec hashing in AddResource).
func xxh3HexOf(data []byte) (string, uint64) {
	if len(data) == 0 {
		return fmt.Sprintf("%016x", uint64(0)), 0
	}

	cData := C.CBytes(data)
	defer C.free(cData)

	hash := uint64(C.XXH3_64bits(cData, C.size_t(len(data))))
	return fmt.Sprintf("%016x", hash), hash
}

// sanitizeResourceNameComponent replaces anything outside [a-zA-Z0-9_.] with '-' so a filename
// or fragment taken verbatim from disk/discovery can't smuggle path separators or other
// surprises into a resource name.
func sanitizeResourceNameComponent(s string) string {
	var b strings.Builder
	for _, r := range s {
		switch {
		case r >= 'a' && r <= 'z', r >= 'A' && r <= 'Z', r >= '0' && r <= '9', r == '_', r == '.':
			b.WriteRune(r)
		default:
			b.WriteRune('-')
		}
	}
	return b.String()
}

// splitManifestFragment splits a discovered path on the first '#', mirroring the
// <filename>.jsonnet#<fragment> URI-fragment convention for selecting a sub-document out of a
// single discovered file.
func splitManifestFragment(path string) (base string, fragment string) {
	base, fragment, _ = strings.Cut(path, "#")
	return base, fragment
}

// deriveManifestResourceName computes the Manifest resource's name per the naming scheme:
//   - Path manifests:    <filename>-<ext>           e.g. packages.yaml -> packages-yaml
//     ...with a fragment: <filename>-<ext>-<fragment> e.g. hosts.jsonnet#laptop -> hosts-jsonnet-laptop
//   - Raw manifests:      raw-<filename>-<ext>
//     Raw manifests have no real filename (discovery only gives us the raw content), so a short
//     hex hash of the content stands in for <filename> -- this keeps the same three-part shape
//     Tazz described rather than inventing a different pattern for the raw case, and it's
//     deterministic (the same raw content always gets the same name) and disambiguates same-kind
//     raw manifests discovered in the same run for free, as a side effect of already needing a
//     content hash for provenance.
func deriveManifestResourceName(dm DiscoveredManifest, contentHash uint64) string {
	ext := strings.ToLower(strings.TrimPrefix(dm.Kind.rawManifestExt(), "."))

	if dm.Kind == DiscoveredManifestPath {
		base, fragment := splitManifestFragment(dm.Value)
		filename := filepath.Base(base)
		stem := strings.TrimSuffix(filename, filepath.Ext(filename))
		pathExt := strings.ToLower(strings.TrimPrefix(filepath.Ext(filename), "."))
		if pathExt == "" {
			pathExt = ext
		}

		name := sanitizeResourceNameComponent(stem) + "-" + pathExt
		if fragment != "" {
			name += "-" + sanitizeResourceNameComponent(fragment)
		}
		return name
	}

	// Raw manifest: no filename to work with, so use a short (8 hex char) content hash in its
	// place -- same "raw-<filename>-<ext>" shape, deterministic per-content.
	shortHash := strconv.FormatUint(contentHash, 16)
	if len(shortHash) > 8 {
		shortHash = shortHash[:8]
	}
	return "raw-" + shortHash + "-" + ext
}

// rawManifestExt returns the file-extension-shaped label for a DiscoveredManifestKind, used both
// as a naming fallback and for the "Source" field's ext component.
func (dmk DiscoveredManifestKind) rawManifestExt() string {
	switch dmk {
	case DiscoveredManifestRawJson:
		return "json"
	case DiscoveredManifestRawYaml:
		return "yaml"
	case DiscoveredManifestRawJsonnet:
		return "jsonnet"
	default:
		return ""
	}
}

// addManifestResource creates a Manifest pseudo-resource in the graph for a single discovered
// manifest entry, in addition to (not instead of) whatever ResourceSpecs that manifest expanded
// into. It never sets depends_on -- Manifest resources don't participate in the dependency graph
// -- and is inserted with RESOURCE_FLAG_SYNTHETIC so it's created kResourceReady and never
// touches the reconcile pipeline.
//
// persist controls whether the resulting resource's state is written to the state store (see
// writeManifestState below). Callers should pass true only for RunApplyMode, mirroring
// IsApplyReconcileTask's gate on WriteResourceState for ordinary resources -- read-only commands
// still get a fully-populated Manifest resource in the graph for this run, they just don't
// persist anything past it.
func (orc *Orchestrator) addManifestResource(dm DiscoveredManifest, resourceCount int, persist bool) error {
	kind := FindResourceKind(manifestResourceKind)
	if kind == InvalidResourceKind {
		return fmt.Errorf("Manifest resource kind is not registered")
	}

	spec := ManifestResourceSpec{
		Source:        dm.Kind.String(),
		ResourceCount: resourceCount,
		DiscoveredAt:  time.Now().Unix(),
	}

	var content []byte
	switch dm.Kind {
	case DiscoveredManifestPath:
		path, fragment := splitManifestFragment(dm.Value)
		spec.Path = dm.Value
		spec.Fragment = fragment

		data, err := os.ReadFile(path)
		if err != nil {
			spec.ReadError = err.Error()
		} else {
			content = data
		}
	default:
		content = []byte(dm.Value)
	}

	hashHex, hashVal := xxh3HexOf(content)
	spec.Hash = hashHex
	if content != nil {
		spec.ContentBase64 = base64.StdEncoding.EncodeToString(content)
	}

	name := deriveManifestResourceName(dm, hashVal)

	rawSpec, err := json.Marshal(spec)
	if err != nil {
		return fmt.Errorf("failed to marshal ManifestResourceSpec for %q: %w", dm.Value, err)
	}

	annotations := []ResourceAnnotation{
		{Key: hyphaHashAnnotationKey, Value: hashHex},
	}

	graph := orc.GetResourceGraph()
	store := C.GetOrcStateStore(orc.Handle)
	res, err := graph.AddResource(
		store,
		kind,
		name,
		coreResourceNamespace,
		nil, // labels
		annotations,
		nil, // depends_on -- Manifest resources never form dependency chains
		string(rawSpec),
		C.kResourceFlagSynthetic,
	)
	if err != nil {
		return err
	}

	if !persist {
		return nil
	}

	return writeManifestState(store, res)
}

// writeManifestState persists a Manifest resource's state, mirroring WriteResourceState in
// reconcile.c field-for-field (see libs/hypha/src/reconcile.c) -- id/kind/name are strdup'd and
// freed after the call since StateStorePut copies them; observed_json/labels/annotations are
// passed by reference like the C side does, since Manifest resources are never mutated after
// being built here.
//
// This is a separate call site from WriteResourceState rather than a shared one because Manifest
// resources never produce a ReconcileTask (no controller, never kResourcePending) for
// WriteResourceState's caller to gate on -- there's no ControllerAction or Reason to speak of,
// which is also why this doesn't touch the history log: HistoryRecord is shaped around an
// action's outcome (action, status, hash_before/after), and nothing ever acts on a Manifest
// resource. The state store's StateEntry has no such assumption -- id/kind/name/hash/observed_json
// is exactly "the last known shape of this thing", which fits a discovery-derived resource fine.
//
// Persisting this also gets a Manifest resource a stable id across runs for free: AddResource
// already resolves through StateStoreFindIdByName(kind, name) before generating a fresh one, so
// once an entry exists here, later `hypha apply` runs reuse the same id rather than generating a
// new one on every invocation.
func writeManifestState(store *C.StateStore, res *C.Resource) error {
	if store == nil {
		return fmt.Errorf("no state store available to persist manifest state")
	}

	var idBuf C.ResourceIdStr
	C.ResourceIdCStr(&res.id, &idBuf[0])
	cID := C.CString(C.GoString(&idBuf[0]))
	defer C.free(unsafe.Pointer(cID))

	kindName := C.FindResourceKindName(res.kind)
	cKind := C.CString(C.GoString(kindName))
	defer C.free(unsafe.Pointer(cKind))

	var cName *C.char
	if res.info.name != nil {
		cName = C.CString(C.GoString(res.info.name))
		defer C.free(unsafe.Pointer(cName))
	}

	entry := C.StateEntry{
		orphaned:        C.bool(false),
		id:              cID,
		kind:            cKind,
		name:            cName,
		hash:            res.spec.hash,
		observed_json:   res.spec.raw,
		last_status:     C.int(res.state),
		applied_at:      C.time(nil),
		labels:          res.info.labels,
		labels_len:      res.info.labels_len,
		annotations:     res.info.annotations,
		annotations_len: res.info.annotations_len,
	}

	if !C.StateStorePut(store, &entry) {
		return fmt.Errorf("failed to write manifest state entry for %s", C.GoString(&idBuf[0]))
	}
	return nil
}
