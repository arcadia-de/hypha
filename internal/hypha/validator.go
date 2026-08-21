package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "hypha.h"
*/
import "C"

import (
	"encoding/json"
	"fmt"
	"os"
	"strings"
	"unsafe"

	hypha_schema "github.com/arcadia-de/hypha/schema"
	"github.com/google/go-jsonnet"
	"github.com/santhosh-tekuri/jsonschema/v6"
)

type SchemaValidator struct {
	Compiler *jsonschema.Compiler
	Schema   *jsonschema.Schema
}

func NewSchemaValidator() (*SchemaValidator, error) {
	compiler := jsonschema.NewCompiler()

	var schema any
	err := json.Unmarshal(hypha_schema.ManifestSchemaJson, &schema)
	if err != nil {
		return nil, fmt.Errorf("failed to unmarshal schema json '%s': %v", string(hypha_schema.ManifestSchemaJson), err)
	}

	err = compiler.AddResource(hypha_schema.ManifestSchemaId, schema)
	if err != nil {
		return nil, fmt.Errorf("schema tracking error: %v", err)
	}

	compiledSchema, err := compiler.Compile(hypha_schema.ManifestSchemaId)
	if err != nil {
		return nil, fmt.Errorf("draft 2020-12 compilation failed: %v", err)
	}

	return &SchemaValidator{
		Compiler: compiler,
		Schema:   compiledSchema,
	}, nil
}

type SchemaValidationResult struct {
	Filename string
	IsValid  bool
	Error    error
}

func (validator *SchemaValidator) ValidateSchemas(vm *jsonnet.VM, filenames []string) []SchemaValidationResult {
	results := make([]SchemaValidationResult, 0)
	for _, filename := range filenames {
		if strings.HasSuffix(filename, ".jsonnet") {
			specs, err := ParseSpecsFromJsonnet(vm, filename)
			if err != nil {
				results = append(results, SchemaValidationResult{
					Filename: filename,
					IsValid:  false,
					Error:    err,
				})
				continue
			}

			for _, spec := range specs {
				err = validator.ValidateSchema(spec.Content)
				if err != nil {
					results = append(results, SchemaValidationResult{
						Filename: spec.Filename,
						IsValid:  false,
						Error:    err,
					})
					continue
				}

				results = append(results, SchemaValidationResult{
					Filename: spec.Filename,
					IsValid:  true,
				})
				continue
			}

			continue
		} else if strings.HasSuffix(filename, "yaml") || strings.HasSuffix(filename, "yml") {
			//TODO(@s0cks): implement
			results = append(results, SchemaValidationResult{
				Filename: filename,
				IsValid:  false,
				Error:    fmt.Errorf("validate yaml files is not implemented"),
			})
			continue
		} else if strings.HasSuffix(filename, "json") {
			manifestBytes, err := os.ReadFile(filename)
			if err != nil {
				results = append(results, SchemaValidationResult{
					Filename: filename,
					IsValid:  false,
					Error:    fmt.Errorf("failed to read manifest: %v", err),
				})
				continue
			}

			var manifest any
			err = json.Unmarshal(manifestBytes, &manifest)
			if err != nil {
				results = append(results, SchemaValidationResult{
					Filename: filename,
					IsValid:  false,
					Error:    fmt.Errorf("failed to unmarhsal manifest: %v", err),
				})
				continue
			}

			err = validator.ValidateSchema(manifest)
			if err != nil {
				results = append(results, SchemaValidationResult{
					Filename: filename,
					IsValid:  false,
					Error:    err,
				})
				continue
			}

			results = append(results, SchemaValidationResult{
				Filename: filename,
				IsValid:  true,
				Error:    nil,
			})
			continue
		}
	}

	return results
}

type ResourceSpecDocument struct {
	Filename string
	Content  any
}

func ParseSpecsFromJsonnet(vm *jsonnet.VM, filename string) ([]ResourceSpecDocument, error) {
	content, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to read manifest: %v", err)
	}

	manifest, err := RenderJsonnetManifest(vm, filename, string(content))
	if err != nil {
		return nil, fmt.Errorf("failed to evaluating manifest Jsonnet: %v", err)
	}

	specs, err := ParseResourceSpecs(manifest)
	if err != nil {
		return nil, fmt.Errorf("failed to parse manifest: %v", err)
	}

	var documents []ResourceSpecDocument
	switch v := specs.(type) {
	case map[string]any:
		isMultiFile := false
		for key := range v {
			kl := strings.ToLower(key)
			if strings.HasSuffix(kl, ".json") || strings.HasSuffix(kl, ".yaml") || strings.HasSuffix(kl, ".yml") {
				isMultiFile = true
				break
			}
		}

		if isMultiFile {
			for fname, fileContent := range v {
				switch childCtx := fileContent.(type) {
				case []any:
					for _, child := range childCtx {
						documents = append(documents, ResourceSpecDocument{
							Filename: fmt.Sprintf("%s#%s", filename, fname),
							Content:  child,
						})
					}
				case map[string]any:
					documents = append(documents, ResourceSpecDocument{
						Filename: fmt.Sprintf("%s#%s", filename, fname),
						Content:  childCtx,
					})
				default:
					return nil, fmt.Errorf("multi-file bundle contains unparseable document data type")
				}
			}
		} else {
			documents = append(documents, ResourceSpecDocument{
				Filename: filename,
				Content:  v,
			})
		}
	}

	return documents, nil
}

func (validator *SchemaValidator) ValidateSchema(inputJson any) error {
	err := validator.Schema.Validate(inputJson)
	if err != nil {
		if valErr, ok := err.(*jsonschema.ValidationError); ok {
			return fmt.Errorf("manifest '%s' is not valid: %v", inputJson, valErr.DetailedOutput())
		} else {
			return fmt.Errorf("manifest '%s' is not valid: %v", inputJson, err)
		}
	}

	return nil
}

//export ValidateManifests
func ValidateManifests(tpls **C.char, num_tpls C.uint64_t, valid *C.bool) C.uint64_t {
	validator, err := NewSchemaValidator()
	if err != nil {
		return C.uint64_t(num_tpls)
	}

	count := uint64(num_tpls)
	goNumInvalid := 0

	validSlice := unsafe.Slice(valid, count)
	tplsSlice := unsafe.Slice(tpls, count)
	for i := range count {
		cTpl := tplsSlice[i]
		goTpl := C.GoString(cTpl)

		var manifest any
		err := json.Unmarshal([]byte(goTpl), &manifest)
		if err != nil {
			validSlice[i] = C.bool(false)
			goNumInvalid++
			continue
		}

		err = validator.ValidateSchema(manifest)
		if err != nil {
			validSlice[i] = C.bool(false)
			goNumInvalid++
			continue
		}

		validSlice[i] = C.bool(true)
	}

	return C.uint64_t(goNumInvalid)
}
