package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include "hypha.h"
#include "hypha/orchestrator.h"
#include "hypha/resource.h"
*/
import "C"

import (
	"encoding/json"
	"fmt"
	"os"
	"unsafe"

	"github.com/google/go-jsonnet"
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

func (orc *Orchestrator) Run() error {
	success := C.OrchestratorRun(orc.Handle)
	if !bool(success) {
		return fmt.Errorf("failed to run Orchestrator")
	}

	return nil
}

func (orc *Orchestrator) RenderGraph(name string, layout string, render string) error {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	cLayout := C.CString(layout)
	defer C.free(unsafe.Pointer(cLayout))

	cRender := C.CString(render)
	defer C.free(unsafe.Pointer(cRender))

	C.OrchestratorRenderResourceGraphToStdout(orc.Handle, cName, cLayout, cRender)
	return nil
}

func (orc *Orchestrator) AddResource(res ResourceSpec) {
	cID := C.CString(res.ID)
	defer C.free(unsafe.Pointer(cID))

	cKind := C.CString(res.Kind)
	defer C.free(unsafe.Pointer(cKind))

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

	specBytes, err := json.Marshal(res.Spec)
	if err != nil {
		fmt.Printf("error marshalling spec: %v", err)
		os.Exit(1)
		return
	}

	cSpec := C.CString(string(specBytes))
	defer C.free(unsafe.Pointer(cSpec))

	spec := C.Resource{
		id:             cID,
		kind:           cKind,
		depends_on:     cDeps,
		num_depends_on: C.uint32_t(numDeps),
		spec:           cSpec,
	}

	C.OrchestratorAddResource(orc.Handle, spec)

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

func (orc *Orchestrator) ParseResourceSpecsFromJsonnet(filename string) ([]ResourceSpec, error) {
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
	C.OrchestratorPrintRuntimeInfo(orc.Handle)
}
