package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "hypha/planner.h"
#include "hypha/resource.h"
#include "hypha/action_log.h"
#include "hypha/annotation.h"
#include "hypha/orchestrator.h"

bool goVisitAppliedActions(uint64_t, AppliedAction*, void*);
bool goVisitDiscoveredManifests(uint64_t, DiscoveredManifest*, void*);
*/
import "C"

import (
	"encoding/json"
	"fmt"
	"os"
	"runtime"
	"runtime/cgo"
	"strings"
	"unsafe"

	lg "charm.land/lipgloss/v2"
	"github.com/google/go-jsonnet"
	"github.com/spf13/viper"
)

type OrchestratorRunMode int

const (
	OrchestratorPlanMode    OrchestratorRunMode = C.kOrchestratorPlanMode
	OrchestratorDiffMode    OrchestratorRunMode = C.kOrchestratorDiffMode
	OrchestratorDestroyMode OrchestratorRunMode = C.kOrchestratorDestroyMode
	OrchestratorApplyMode   OrchestratorRunMode = C.kOrchestratorApplyMode
)

type Orchestrator struct {
	Handle  C.OrchestratorHandle
	Jsonnet *jsonnet.VM
}

type OrchestratorConfig struct {
	Root     string
	CacheDir string
	StateDir string
}

type OrchestratorMetrics struct {
	Start     uint64 `json:"start" yaml:"start"`
	Finish    uint64 `json:"finish" yaml:"finish"`
	Processed uint64 `json:"processed" yaml:"processed"`
	Noop      uint64 `json:"noop" yaml:"noop"`
	Created   uint64 `json:"created" yaml:"created"`
	Updated   uint64 `json:"updated" yaml:"updated"`
	Destroyed uint64 `json:"destroyed" yaml:"destroyed"`
}

func NewOrchestrator(config OrchestratorConfig) *Orchestrator {
	vm := CreateJsonnetVM()
	cRoot := C.CString(config.Root)
	defer C.free(unsafe.Pointer(cRoot))

	cStateDir := C.CString(config.StateDir)
	defer C.free(unsafe.Pointer(cStateDir))

	cCacheDir := C.CString(config.CacheDir)
	defer C.free(unsafe.Pointer(cCacheDir))

	cConfig := C.OrchestratorConfig{
		root:      cRoot,
		state_dir: cStateDir,
		cache_dir: cCacheDir,
	}
	orc := C.NewOrchestrator(cConfig)

	return &Orchestrator{
		Handle:  orc,
		Jsonnet: vm,
	}
}

func (orc *Orchestrator) GetMetrics() OrchestratorMetrics {
	var cMetrics C.OrchestratorMetrics
	C.GetOrchestratorMetrics(orc.Handle, &cMetrics)
	return OrchestratorMetrics{
		Start:     uint64(cMetrics.run_start),
		Finish:    uint64(cMetrics.run_finished),
		Processed: uint64(cMetrics.num_processed),
		Noop:      uint64(cMetrics.num_actions[C.kNoAction]),
		Created:   uint64(cMetrics.num_actions[C.kCreateAction]),
		Updated:   uint64(cMetrics.num_actions[C.kUpdateAction]),
		Destroyed: uint64(cMetrics.num_actions[C.kDestroyAction]),
	}
}

func (orc *Orchestrator) PrintMetrics() error {
	metrics := orc.GetMetrics()
	fmt.Println("Telemetry:")
	s, err := json.Marshal(metrics)
	if err != nil {
		return fmt.Errorf("failed to marshal OrchestratorMetrics: %v", err)
	}
	fmt.Println(string(s))
	return nil
}

func (orc *Orchestrator) PrintMetricsIfDesired() error {
	telemetry := viper.GetBool("print-telemetry")
	if telemetry {
		return orc.PrintMetrics()
	}

	return nil
}

func NewOrchestratorWithDefaultConfig() (*Orchestrator, error) {
	config_dir, err := EnsureConfigDirExists()
	if err != nil {
		return nil, err
	}

	state_dir, err := EnsureStateDirExists()
	if err != nil {
		return nil, err
	}

	cache_dir, err := EnsureCacheDirExists()
	if err != nil {
		return nil, err
	}

	config := OrchestratorConfig{
		Root:     config_dir,
		StateDir: state_dir,
		CacheDir: cache_dir,
	}
	return NewOrchestrator(config), nil
}

func (orc *Orchestrator) EvalLuaExpr(code string) error {
	cCode := C.CString(code)
	defer C.free(unsafe.Pointer(cCode))

	var err_message *C.char
	if !C.OrchestratorEvalExpr(orc.Handle, cCode, &err_message) {
		defer C.free(unsafe.Pointer(err_message))

		gErr := C.GoString(err_message)
		return fmt.Errorf("failed to evaluate lua expr '%s': %s", code, gErr)
	}

	return nil
}

func (orc *Orchestrator) EvalLuaFile(filename string) error {
	cFilename := C.CString(filename)
	defer C.free(unsafe.Pointer(cFilename))

	var err_message *C.char
	if !C.OrchestratorEvalFile(orc.Handle, cFilename, &err_message) {
		gErr := C.GoString(err_message)
		C.free(unsafe.Pointer(err_message))
		return fmt.Errorf("failed to evaluate lua file '%s': %s", filename, gErr)
	}

	return nil
}

func (orc *Orchestrator) RenderJsonnetManifest(name string, code string) (string, error) {
	return orc.Jsonnet.EvaluateAnonymousSnippet(name, code)
}

func (orc *Orchestrator) RenderAnonymousJsonnetManifest(code string) (string, error) {
	return orc.RenderJsonnetManifest("manifest.jsonnet", code)
}

func (orc *Orchestrator) Run(mode OrchestratorRunMode) error {
	success := C.OrchestratorRun(orc.Handle, C.OrchestratorRunMode(mode))
	if !bool(success) {
		return fmt.Errorf("failed to run Orchestrator")
	}

	return nil
}

func (orc *Orchestrator) RunWithReason(mode OrchestratorRunMode, reason string) error {
	cReason := C.CString(reason)
	defer C.free(unsafe.Pointer(cReason))

	success := C.OrchestratorRunWithReason(orc.Handle, C.OrchestratorRunMode(mode), cReason)
	if !bool(success) {
		return fmt.Errorf("failed to run Orchestrator")
	}

	return nil
}

func (orc *Orchestrator) AddResource(res ResourceSpec) {
	cKind := C.CString(res.Kind)
	defer C.free(unsafe.Pointer(cKind))

	cName := C.CString(res.Metadata.Name)
	defer C.free(unsafe.Pointer(cName))

	var cDeps **C.char
	numDeps := len(res.DependsOn)
	if numDeps > 0 {
		cDepsSlice := C.malloc(C.size_t(numDeps) * C.size_t(unsafe.Sizeof(uintptr(0))))
		cDeps = (**C.char)(cDepsSlice)

		goSlice := (*[1 << 30]*C.char)(cDepsSlice)[:numDeps:numDeps]
		for i, dep := range res.DependsOn {
			goSlice[i] = C.CString(dep)
		}
	}

	var cLabels *C.Label
	numLabels := len(res.Metadata.Labels)
	if numLabels > 0 {
		var dummyLabel C.Label
		labelSize := int(unsafe.Sizeof(dummyLabel))
		cLabelsSlice := C.malloc(C.size_t(numLabels) * C.size_t(labelSize))
		cLabels = (*C.Label)(cLabelsSlice)
		C.memset(cLabelsSlice, 0, C.size_t(numLabels)*C.size_t(labelSize))
		rawByteSlice := unsafe.Slice((*byte)(cLabelsSlice), numLabels*labelSize)

		for i, labelVal := range res.Metadata.Labels {
			offset := i * labelSize
			maxSafeLength := labelSize - 1

			if len(labelVal) > maxSafeLength {
				labelVal = labelVal[:maxSafeLength]
			}

			copy(rawByteSlice[offset:offset+len(labelVal)], labelVal)
		}
	}

	var cAnnotations *C.Annotation
	numAnnos := len(res.Metadata.Annotations)
	if numAnnos > 0 {
		var dummyAnno C.Annotation
		annoSize := int(unsafe.Sizeof(dummyAnno))

		cAnnosSlice := C.malloc(C.size_t(numAnnos) * C.size_t(annoSize))
		cAnnotations = (*C.Annotation)(cAnnosSlice)

		C.memset(cAnnosSlice, 0, C.size_t(numAnnos)*C.size_t(annoSize))
		rawByteSlice := unsafe.Slice((*byte)(cAnnosSlice), numAnnos*annoSize)

		keySize := int(C.HYPHA_ANNOTATION_KEY_SIZE)
		valSize := int(C.HYPHA_ANNOTATION_VALUE_SIZE)

		for i, anno := range res.Metadata.Annotations {
			structOffset := i * annoSize

			keyStart := structOffset
			valStart := structOffset + keySize

			maxKeyLen := keySize - 1
			maxValLen := valSize - 1

			keyStr := anno.Key
			if len(keyStr) > maxKeyLen {
				keyStr = keyStr[:maxKeyLen]
			}

			valStr := anno.Value
			if len(valStr) > maxValLen {
				valStr = valStr[:maxValLen]
			}

			copy(rawByteSlice[keyStart:keyStart+len(keyStr)], keyStr)
			copy(rawByteSlice[valStart:valStart+len(valStr)], valStr)
		}
	}

	var cRawSpec *C.char
	if res.Spec != nil {
		specBytes, err := json.Marshal(res.Spec)
		if err != nil {
			fmt.Printf("error marshalling spec: %v", err)
			os.Exit(1)
			return
		}
		cRawSpec = C.CString(string(specBytes))
	}
	defer C.free(unsafe.Pointer(cRawSpec))

	spec := C.Resource{
		kind:           cKind,
		depends_on:     cDeps,
		num_depends_on: C.size_t(numDeps),
		info: C.ResourceInfo{
			name:       cName,
			labels:     cLabels,
			labels_len: C.size_t(numLabels),
			labels_cap: C.size_t(numLabels),

			annotations:     cAnnotations,
			annotations_len: C.size_t(numAnnos),
			annotations_cap: C.size_t(numAnnos),
		},
		spec: C.ResourceSpecDocument{
			raw: cRawSpec,
			doc: nil,
		},
	}

	C.OrchestratorAddResource(orc.Handle, &spec)

	if numDeps > 0 {
		goSlice := (*[1 << 30]*C.char)(unsafe.Pointer(cDeps))[:numDeps:numDeps]
		for slice := range goSlice {
			C.free(unsafe.Pointer(goSlice[slice]))
		}

		C.free(unsafe.Pointer(cDeps))
	}
}

func (orc *Orchestrator) Close() {
	C.FreeOrchestrator(orc.Handle)
}

type DiscoveredManifestKind int

const (
	DiscoveredManifestPath = C.kDiscoveredPath
	DiscoveredManifestRaw  = C.kDiscoveredRaw
)

type DiscoveredManifest struct {
	Kind  DiscoveredManifestKind
	Value string
}

type DiscoveredManifestVisitor func(idx uint64, manifest DiscoveredManifest) bool

//export goVisitDiscoveredManifests
func goVisitDiscoveredManifests(idx C.uint64_t, dm *C.DiscoveredManifest, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(DiscoveredManifestVisitor)

	goManifest := DiscoveredManifest{
		Kind:  DiscoveredManifestKind(dm.kind),
		Value: C.GoString(dm.value),
	}
	return C.bool(vis(uint64(idx), goManifest))
}

func (orc *Orchestrator) VisitDiscoveredManifests(vis DiscoveredManifestVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	C.VisitDiscoveredManifests(
		orc.Handle,
		(C.VisitDiscoveredManifestFn)(unsafe.Pointer(C.goVisitDiscoveredManifests)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}

type AppliedAction struct {
	Action uint32
	Name   string
	Kind   string
	Reason string
}

type AppliedActionVisitor func(idx uint64, act AppliedAction) bool

//export goVisitAppliedActions
func goVisitAppliedActions(idx C.uint64_t, act *C.AppliedAction, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(AppliedActionVisitor)

	goName := C.GoString(act.resource.info.name)
	goKind := C.GoString(act.resource.kind)

	rawReason := C.GoStringN(&act.reason[0], C.int(C.HYPHA_REASON_MAX_LENGTH))
	goReason, _, _ := strings.Cut(rawReason, "\x00")
	goAction := AppliedAction{
		Action: uint32(act.action),
		Name:   goName,
		Kind:   goKind,
		Reason: goReason,
	}
	return C.bool(vis(uint64(idx), goAction))
}

func (orc *Orchestrator) VisitAppliedActions(vis AppliedActionVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	C.VisitAllActions(
		C.GetOrcActionLog(orc.Handle),
		(C.VisitActionFn)(unsafe.Pointer(C.goVisitAppliedActions)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}

func (orc *Orchestrator) ParseResourceSpecsFromJsonnet(content string) ([]ResourceSpec, error) {
	manifest, err := orc.RenderAnonymousJsonnetManifest(content)
	if err != nil {
		return nil, fmt.Errorf("failed to evaluating manifest Jsonnet: %v", err)
	}

	var results []ResourceSpec
	specs, err := ParseResourceSpecs(manifest)
	if err != nil {
		return nil, fmt.Errorf("failed to parse manifest: %v", err)
	}

	switch v := specs.(type) {
	case []any:
		bytes, _ := json.Marshal(v)
		if err := json.Unmarshal(bytes, &results); err != nil {
			return nil, err
		}
	case map[string]any:
		bytes, _ := json.Marshal(v)
		var single ResourceSpec
		if err := json.Unmarshal(bytes, &single); err != nil {
			return nil, err
		}

		results = append(results, single)
	default:
		return nil, fmt.Errorf("failed to parse resource spec")
	}

	return results, nil
}

func (orc *Orchestrator) ParseResourceSpecsFromJsonnetFile(filename string) ([]ResourceSpec, error) {
	content, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to read manifest: %v", err)
	}

	manifest, err := orc.RenderAnonymousJsonnetManifest(string(content))
	if err != nil {
		return nil, fmt.Errorf("failed to evaluating manifest Jsonnet: %v", err)
	}

	var results []ResourceSpec
	specs, err := ParseResourceSpecs(manifest)
	if err != nil {
		return nil, fmt.Errorf("failed to parse manifest: %v", err)
	}

	switch v := specs.(type) {
	case []any:
		bytes, _ := json.Marshal(v)
		if err := json.Unmarshal(bytes, &results); err != nil {
			return nil, err
		}
	case map[string]any:
		bytes, _ := json.Marshal(v)
		var single ResourceSpec
		if err := json.Unmarshal(bytes, &single); err != nil {
			return nil, err
		}

		results = append(results, single)
	default:
		return nil, fmt.Errorf("failed to parse resource spec")
	}

	return results, nil
}

func (orc *Orchestrator) CollectGarbage() error {
	// if !C.OrchestratorPruneOrphans(orc.Handle) {
	// 	return fmt.Errorf("failed to prune orchestrator orphans")
	// }

	if !C.OrchestratorCompact(orc.Handle) {
		return fmt.Errorf("failed to compact orchestrator log")
	}

	return nil
}

func (orc *Orchestrator) PrintRuntimeInfo() {
	fmt.Println()
	rowStyle := lg.NewStyle().
		MarginLeft(2)

	fmt.Println(rowStyle.Render("Hypha Runtime Info:"))

	rowStyle = rowStyle.MarginLeft(4)
	goConfigDir := C.GoString(C.GetOrcConfigDir(orc.Handle))
	goStateDir := C.GoString(C.GetOrcStateDir(orc.Handle))
	goCacheDir := C.GoString(C.GetOrcCacheDir(orc.Handle))

	keyStyle := lg.NewStyle().
		Bold(true)

	valueStyle := lg.NewStyle()
	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("Config Dir"),
		": ",
		valueStyle.Render(goConfigDir),
	)))
	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("State Dir"),
		": ",
		valueStyle.Render(goStateDir),
	)))
	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("Cache Dir"),
		": ",
		valueStyle.Render(goCacheDir),
	)))

	luaVersion := C.lua_version(C.GetOrcLuaState(orc.Handle))
	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("lua version"),
		": ",
		valueStyle.Render(fmt.Sprintf("%.1f", luaVersion)),
	)))
	goLibuvVersion := C.GoString(C.uv_version_string())
	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("libuv version"),
		": ",
		valueStyle.Render(goLibuvVersion),
	)))

	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("Registered Controllers"),
		":",
	)))
	ctrlRowStyle := rowStyle.
		MarginLeft(6)
	VisitControllers(func(ctrl Controller) bool {
		fmt.Println(ctrlRowStyle.Render(
			fmt.Sprintf("- %s", ctrl.Kind),
		))
		return true
	})

	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("Service Managers"),
		":",
	)))
	smRowStyle := rowStyle.
		MarginLeft(6)
	VisitAllServiceManagers(func(sm ServiceManager) bool {
		fmt.Println(smRowStyle.Render(fmt.Sprintf("- %s", sm.GetName())))
		return true
	})

	manifestRowStyle := rowStyle.
		MarginLeft(6)
	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("Discovered Manifests"),
		":",
	)))
	orc.VisitDiscoveredManifests(func(idx uint64, manifest DiscoveredManifest) bool {
		fmt.Println(manifestRowStyle.Render(
			fmt.Sprintf("- %s", manifest.Value),
		))
		return true
	})

	fmt.Println()
}

func (orc *Orchestrator) GetPlan() *Plan {
	cHandle := C.GetOrcPlan(orc.Handle)
	if cHandle == nil {
		return nil
	}

	return &Plan{
		Handle: cHandle,
	}
}

func (orc *Orchestrator) GetValidationLog() ValidationLog {
	return GetValidationLog(C.GetOrcValidationLog(orc.Handle))
}

func (orc *Orchestrator) ProcessDiscoveredManifests() {
	var manifests []ResourceSpec
	orc.VisitDiscoveredManifests(func(idx uint64, dm DiscoveredManifest) bool {
		var specs []ResourceSpec
		var err error

		switch dm.Kind {
		case DiscoveredManifestPath:
			if strings.HasSuffix(dm.Value, ".yaml") {
				//TODO(@s0cks): implement
			} else if strings.HasSuffix(dm.Value, ".jsonnet") {
				specs, err = orc.ParseResourceSpecsFromJsonnetFile(dm.Value)
			}
		case DiscoveredManifestRaw:
			specs, err = orc.ParseResourceSpecsFromJsonnet(dm.Value)
		}

		if err != nil {
			fmt.Printf("error processing %s: %v", dm.Value, err)
			return false
		}

		for _, s := range specs {
			manifests = append(manifests, s)
		}
		return true
	})

	for _, s := range manifests {
		orc.AddResource(s)
	}
}

func (orc *Orchestrator) GetResourceGraph() ResourceGraph {
	return ResourceGraph{
		Handle: C.GetOrcResourceGraph(orc.Handle),
	}
}
