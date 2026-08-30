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

func (orc *Orchestrator) GetResourceGraph() ResourceGraph {
	return ResourceGraph{
		Handle: C.GetOrcResourceGraph(orc.Handle),
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
		// } else if config_dir == nil {
		// 	return nil, fmt.Errorf("state dir is nil")
	}

	cache_dir, err := EnsureCacheDirExists()
	if err != nil {
		return nil, err
		// } else if cache_dir == nil {
		// 	return nil, fmt.Errorf("cache dir is nil")
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

func (orc *Orchestrator) Run(info RunInfo) error {
	i := info.ToC()
	defer C.free(unsafe.Pointer(i))

	success := C.OrchestratorRun(orc.Handle, i)
	if !bool(success) {
		return fmt.Errorf("failed to run Orchestrator")
	}

	return nil
}

func (orc *Orchestrator) AddResource(spec ResourceSpec) error {
	kind := FindResourceKind(spec.Kind)
	if kind == InvalidResourceKind {
		return fmt.Errorf("unknown resource kind %q", spec.Kind)
	}

	ns := spec.Metadata.Namespace
	if IsReservedResourceNamespace(ns) {
		return fmt.Errorf("rejected resource %s/%s: namespace %q is reserved for orchestrator-internal use",
			spec.Kind, spec.Metadata.Name, ns)
	}

	var rawSpec string
	if spec.Spec != nil {
		specBytes, err := json.Marshal(spec.Spec)
		if err != nil {
			return fmt.Errorf("error marshalling spec: %v", err)
		}
		rawSpec = string(specBytes)
	}

	graph := orc.GetResourceGraph()
	store := C.GetOrcStateStore(orc.Handle)
	return graph.AddResource(store, kind, spec.Metadata.Name, ns, spec.Metadata.Labels, spec.Metadata.Annotations, spec.DependsOn, rawSpec)
}

func (orc *Orchestrator) Close() {
	C.FreeOrchestrator(orc.Handle)
}

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

func (orc *Orchestrator) GetActionLog() *AppliedActionLog {
	handle := C.GetOrcAppliedActionLog(orc.Handle)
	if handle == nil {
		return nil
	}

	return &AppliedActionLog{
		Handle: handle,
	}
}

func (orc *Orchestrator) ParseResourceSpecsFromJsonnet(content string) ([]ResourceSpec, error) {
	manifest, err := orc.RenderAnonymousJsonnetManifest(content)
	if err != nil {
		return nil, fmt.Errorf("failed to evaluating manifest Jsonnet: %v", err)
	}

	specs, err := ParseResourceSpecs(manifest)
	if err != nil {
		return nil, fmt.Errorf("failed to parse manifest: %v", err)
	}

	return ResourceSpecsFromAny(specs)
}

func (orc *Orchestrator) ParseResourceSpecsFromJsonnetFile(filename string) ([]ResourceSpec, error) {
	content, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to read manifest: %v", err)
	}

	return orc.ParseResourceSpecsFromJsonnet(string(content))
}

func (orc *Orchestrator) ParseResourceSpecsFromPath(path string) ([]ResourceSpec, error) {
	switch DetectManifestFormat(path) {
	case ManifestFormatYAML:
		docs, err := ParseResourceSpecsFromYamlFile(path)
		if err != nil {
			return nil, err
		}
		return ResourceSpecsFromDocuments(docs)
	case ManifestFormatJSON:
		docs, err := ParseResourceSpecsFromJsonFile(path)
		if err != nil {
			return nil, err
		}
		return ResourceSpecsFromDocuments(docs)
	case ManifestFormatJsonnet:
		return orc.ParseResourceSpecsFromJsonnetFile(path)
	default:
		return nil, fmt.Errorf("unsupported manifest format for %q: expected .yaml, .yml, .json, or .jsonnet", path)
	}
}

func (orc *Orchestrator) ParseResourceSpecsFromString(content string, format ManifestFormat) ([]ResourceSpec, error) {
	switch format {
	case ManifestFormatYAML:
		docs, err := ParseResourceSpecsFromYaml(content)
		if err != nil {
			return nil, err
		}
		return ResourceSpecsFromDocuments(docs)
	case ManifestFormatJSON:
		docs, err := ParseResourceSpecsFromJson(content)
		if err != nil {
			return nil, err
		}
		return ResourceSpecsFromDocuments(docs)
	case ManifestFormatJsonnet:
		return orc.ParseResourceSpecsFromJsonnet(content)
	default:
		return nil, fmt.Errorf("unsupported raw manifest %s", content)
	}
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

	luaPath := os.Getenv("LUA_PATH")
	luaRowStyle := rowStyle.MarginLeft(rowStyle.GetMarginLeft() + 2)
	fmt.Println(luaRowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		keyStyle.Render("LUA_PATH"),
		": ",
		valueStyle.Render(luaPath),
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
		keyStyle.Render("Resource Kinds"),
		":",
	)))
	kindRowStyle := rowStyle.
		MarginLeft(6)
	VisitAllResourceKinds(func(info ResourceKindInfo) bool {
		fmt.Println(kindRowStyle.Render(
			fmt.Sprintf("- %s (%d)", info.Name, info.Kind),
		))
		return true
	})

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

func (orc *Orchestrator) ProcessDiscoveredManifests() error {
	validator, err := NewSchemaValidator()
	if err != nil {
		return fmt.Errorf("failed to initialize schema validator: %w", err)
	}

	var manifests []ResourceSpec
	orc.VisitDiscoveredManifests(func(idx uint64, dm DiscoveredManifest) bool {
		var err error
		var specs []ResourceSpec

		switch dm.Kind {
		case DiscoveredManifestPath:
			specs, err = orc.ParseResourceSpecsFromPath(dm.Value)
		case DiscoveredManifestRawJson:
			specs, err = orc.ParseResourceSpecsFromString(dm.Value, ManifestFormatJSON)
		case DiscoveredManifestRawJsonnet:
			specs, err = orc.ParseResourceSpecsFromString(dm.Value, ManifestFormatJsonnet)
		case DiscoveredManifestRawYaml:
			specs, err = orc.ParseResourceSpecsFromString(dm.Value, ManifestFormatYAML)
		default:
			fmt.Printf("error process: %s (%s)", dm.Value, dm.Kind.String())
			return false
		}

		if err != nil {
			fmt.Printf("error processing %s (%s): %v", dm.Value, dm.Kind.String(), err)
			return false
		}

		if len(specs) == 0 {
			fmt.Printf("empty manifest: %s\n", dm.Value)
			return false
		}

		for _, s := range specs {
			manifests = append(manifests, s)
		}

		return true
	})

	for _, s := range manifests {
		if err := validator.ValidateResourceSpec(s); err != nil {
			fmt.Printf("skipping resource %q (%s): %v\n", s.Metadata.Name, s.Kind, err)
			continue
		}

		orc.AddResource(s)
	}

	return nil
}
