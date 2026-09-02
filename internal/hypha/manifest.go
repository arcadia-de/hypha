package hypha

import (
	"encoding/json"
	"fmt"
	"github.com/google/go-jsonnet"
	"gopkg.in/yaml.v3"
	"os"
	"path/filepath"
	"strings"
)

type ManifestFormat int

const (
	ManifestFormatUnknown ManifestFormat = iota
	ManifestFormatYAML
	ManifestFormatJSON
	ManifestFormatJsonnet
)

func (mf ManifestFormat) String() string {
	switch mf {
	case ManifestFormatYAML:
		return "Yaml"
	case ManifestFormatJSON:
		return "Json"
	case ManifestFormatJsonnet:
		return "Jsonnet"
	case ManifestFormatUnknown:
		return "Unknown"
	default:
		return "Unknown"
	}
}

func DetectManifestFormat(path string) ManifestFormat {
	switch strings.ToLower(filepath.Ext(path)) {
	case ".yaml", ".yml":
		return ManifestFormatYAML
	case ".json":
		return ManifestFormatJSON
	case ".jsonnet", ".libsonnet":
		return ManifestFormatJsonnet
	default:
		return ManifestFormatUnknown
	}
}

type ResourceAnnotation struct {
	Key   string `json:"key" yaml:"key"`
	Value string `json:"value" yaml:"value"`
}

type ResourceMetadata struct {
	Name        string               `json:"name,omitempty" yaml:"name,omitempty"`
	Namespace   string               `json:"namespace,omitempty" yaml:"namespace,omitempty"`
	Labels      []string             `json:"labels,omitempty" yaml:"labels,omitempty"`
	Annotations []ResourceAnnotation `json:"annotations,omitempty" yaml:"annotations,omitempty"`
}

type ResourceSpec struct {
	Kind      string           `json:"kind" yaml:"kind"`
	Metadata  ResourceMetadata `json:"metadata" yaml:"metadata"`
	DependsOn []string         `json:"depends_on,omitempty" yaml:"depends_on,omitempty"`
	Spec      any              `json:"spec" yaml:"spec"`
	Version   string           `json:"version,omitempty" yaml:"version,omitempty"`
}

func ParseResourceSpecs(code string) (any, error) {
	var spec any
	err := yaml.Unmarshal([]byte(code), &spec)
	if err != nil {
		return nil, err
	}

	return spec, nil
}

func RenderJsonnetManifest(vm *jsonnet.VM, name string, code string) (string, error) {
	return vm.EvaluateAnonymousSnippet(name, code)
}

func ParseResourceSpecsFromYaml(content string) ([]any, error) {
	var node yaml.Node
	if err := yaml.Unmarshal([]byte(content), &node); err != nil {
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

func ParseResourceSpecsFromYamlFile(filename string) ([]any, error) {
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

func ParseResourceSpecsFromJsonFile(filename string) ([]any, error) {
	content, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to read manifest: %v", err)
	}

	var root any
	if err := json.Unmarshal(content, &root); err != nil {
		return nil, fmt.Errorf("failed to parse json: %v", err)
	}

	switch v := root.(type) {
	case []any:
		return v, nil
	case map[string]any:
		return []any{v}, nil
	default:
		return nil, fmt.Errorf("unsupported manifest root structure")
	}
}

func ParseResourceSpecsFromJson(content string) ([]any, error) {
	var root any
	if err := json.Unmarshal([]byte(content), &root); err != nil {
		return nil, fmt.Errorf("failed to parse json: %v", err)
	}

	switch v := root.(type) {
	case []any:
		return v, nil
	case map[string]any:
		return []any{v}, nil
	default:
		return nil, fmt.Errorf("unsupported manifest root structure")
	}
}

func ResourceSpecsFromDocuments(docs []any) ([]ResourceSpec, error) {
	bytes, err := json.Marshal(docs)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal manifest documents: %w", err)
	}

	var results []ResourceSpec
	if err := json.Unmarshal(bytes, &results); err != nil {
		return nil, fmt.Errorf("failed to decode manifest documents: %w", err)
	}
	return results, nil
}

func ResourceSpecsFromAny(specs any) ([]ResourceSpec, error) {
	switch v := specs.(type) {
	case []any:
		return ResourceSpecsFromDocuments(v)
	case map[string]any:
		return ResourceSpecsFromDocuments([]any{v})
	default:
		return nil, fmt.Errorf("failed to parse resource spec")
	}
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
