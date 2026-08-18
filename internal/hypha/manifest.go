package hypha

import (
	"encoding/json"
	"fmt"
	"github.com/google/go-jsonnet"
	"github.com/google/go-jsonnet/ast"
	"gopkg.in/yaml.v3"
	"os"
	"runtime"
)

type ResourceAnnotation struct {
	Key   string `json:"key" yaml:"key"`
	Value string `json:"value" yaml:"value"`
}

type ResourceMetadata struct {
	Name        string               `json:"name,omitempty" yaml:"name,omitempty"`
	Labels      []string             `json:"labels,omitempty" yaml:"labels,omitempty"`
	Annotations []ResourceAnnotation `json:"annotations,omitempty" yaml:"annotations,omitempty"`
}

type ResourceSpec struct {
	ID        string           `json:"id,omitempty" yaml:"id,omitempty"`
	Kind      string           `json:"kind" yaml:"kind"`
	DependsOn []string         `json:"depends_on,omitempty" yaml:"depends_on,omitempty"`
	Metadata  ResourceMetadata `json:"metadata" yaml:"metadata"`
	Spec      any              `json:"spec" yaml:"spec"`
}

type Resource struct {
	ID       string `json:"id,omitempty" yaml:"id,omitempty"`
	Kind     string `json:"kind,omitempty" yaml:"kind,omitempty"`
	State    string `json:"state,omitempty" yaml:"state,omitempty"`
	Action   string
	Reason   string
	Metadata ResourceMetadata `json:"metadata" yaml:"metadata"`
	Spec     string
}

func ParseResourceSpecs(code string) (any, error) {
	var spec any
	err := yaml.Unmarshal([]byte(code), &spec)
	if err != nil {
		return nil, err
	}

	return spec, nil
}

type MemoryImporter struct{}

func (m *MemoryImporter) Import(importedFrom, importedPath string) (contents jsonnet.Contents, foundAt string, err error) {
	if importedPath == "shared_config" {
		return jsonnet.MakeContents(`
			local getOperatingSystemName = std.native("getOperatingSystemName");
			{
				getOperatingSystemName: getOperatingSystemName,
			}
		`), "memory", nil
	}
	return jsonnet.Contents{}, "", fmt.Errorf("import blocked: %s", importedPath)
}

func HandleCubeFunc(args []any) (any, error) {
	num, ok := args[0].(float64)
	if !ok {
		return nil, fmt.Errorf("argument must be a number")
	}

	return num * num * num, nil
}

func CubeFunc() *jsonnet.NativeFunction {
	return &jsonnet.NativeFunction{
		Name:   "cube",
		Params: []ast.Identifier{"x"},
		Func:   HandleCubeFunc,
	}
}

func HandleGetOperatingSystemName(args []any) (any, error) {
	return runtime.GOOS, nil
}

func GetOperatingSystemName() *jsonnet.NativeFunction {
	return &jsonnet.NativeFunction{
		Name: "getOperatingSystemName",
		Func: HandleGetOperatingSystemName,
	}
}

func CreateJsonnetVM() *jsonnet.VM {
	vm := jsonnet.MakeVM()
	vm.Importer(&MemoryImporter{})
	vm.ExtVar("env", "production")
	vm.ExtCode("features", `{"enableBeta": true, "maxUsers": 100}`)

	vm.NativeFunction(CubeFunc())
	vm.NativeFunction(GetOperatingSystemName())
	return vm
}

func RenderJsonnetManifest(vm *jsonnet.VM, name string, code string) (string, error) {
	return vm.EvaluateAnonymousSnippet(name, code)
}

func ParseResourceSpecsFromYaml(filename string) ([]any, error) {
	content, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to read manifest: %v", err)
	}

	var node yaml.Node
	if err := yaml.Unmarshal(content, &node); err != nil {
		return nil, fmt.Errorf("failed to parse yaml: %v", err)
	}

	var results []any
	switch node.Content[0].Kind {
	case yaml.SequenceNode:
		var slice []map[string]any
		if err := node.Content[0].Decode(&slice); err != nil {
			return nil, fmt.Errorf("failed decoding manifest sequence: %w", err)
		}
		for _, item := range slice {
			results = append(results, item)
		}

	case yaml.MappingNode:
		var doc map[string]any
		if err := node.Content[0].Decode(&doc); err != nil {
			return nil, fmt.Errorf("failed decoding manifest object: %w", err)
		}
		results = append(results, doc)

	default:
		return nil, fmt.Errorf("unsupported manifest root structure")
	}

	return results, nil
}

func ParseResourceSpecsFromJsonnet(vm *jsonnet.VM, filename string) ([]any, error) {
	content, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to read manifest: %v", err)
	}

	manifest, err := RenderJsonnetManifest(vm, filename, string(content))
	if err != nil {
		return nil, fmt.Errorf("failed to evaluating manifest Jsonnet: %v", err)
	}

	var results []any
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
		var single any
		if err := json.Unmarshal(bytes, &single); err != nil {
			return nil, err
		}

		results = append(results, single)
	default:
		return nil, fmt.Errorf("failed to parse resource spec")
	}

	return results, nil
}

func WriteManifestToYamlFile(filename string, spec ResourceSpec) error {
	return fmt.Errorf("not implemented")
}

func WriteManifestToJsonFile(filename string, spec ResourceSpec) error {
	file, err := os.Create(filename)
	if err != nil {
		return fmt.Errorf("failed to create %s file: %v", filename, err)
	}
	defer file.Close()

	encoder := json.NewEncoder(file)
	encoder.SetIndent("", "  ")
	if err := encoder.Encode(spec); err != nil {
		return fmt.Errorf("failed to encode json: %w", err)
	}
	return nil
}

func WriteManifestToFile(filename string, format string, spec ResourceSpec) error {
	switch format {
	case "json":
		return WriteManifestToJsonFile(filename, spec)
	case "yaml":
		return WriteManifestToYamlFile(filename, spec)
	default:
		break
	}

	return fmt.Errorf("invalid output format: %s", format)
}
