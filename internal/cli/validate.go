package cli

import (
	"encoding/json"
	"fmt"
	"os"
	"strings"

	"github.com/google/jsonschema-go/jsonschema"
	"github.com/spf13/cobra"

	"github.com/arcadia-de/hypha/internal/hypha"
	hypha_schema "github.com/arcadia-de/hypha/schema"
)

func GetManifestSchema() (*jsonschema.Resolved, error) {
	var schema jsonschema.Schema
	if err := json.Unmarshal(hypha_schema.ManifestSchemaJson, &schema); err != nil {
		return nil, fmt.Errorf("failed to parse manifest json schema: %v", err)
	}

	resolved, err := schema.Resolve(nil)
	if err != nil {
		return nil, fmt.Errorf("failed to resolve manifest schema references: %v", err)
	}

	return resolved, nil
}

func handleValidate(cmd *cobra.Command, args []string) error {
	resolved, err := GetManifestSchema()
	if err != nil {
		return err
	}

	vm := hypha.CreateJsonnetVM()

	for i := range args {
		filename := args[i]
		fmt.Printf("validating: %s....", filename)

		if strings.HasSuffix(filename, ".jsonnet") {
			specs, err := hypha.ParseResourceSpecsFromJsonnet(vm, filename)
			if err != nil {
				return err
			}

			for i := range specs {
				if err := resolved.Validate(specs[i]); err != nil {
					return fmt.Errorf("Validation Failed: %v\n", err)
				}
			}
		} else if strings.HasSuffix(filename, "yaml") || strings.HasSuffix(filename, "yml") {
			content, err := os.ReadFile(filename)
			if err != nil {
				return fmt.Errorf("failed to read manifest: %v", err)
			}

			specs, err := hypha.ParseResourceSpecsFromYaml(string(content))
			for i := range specs {
				if err := resolved.Validate(specs[i]); err != nil {
					return fmt.Errorf("Validation Failed: %v\n", err)
				}
			}
		}
	}

	fmt.Println("Manifest is valid")
	return nil
}

var validateCmd = &cobra.Command{
	Use:     "validate [manifests]",
	Short:   "Validate the specified manifests",
	Args:    cobra.MinimumNArgs(1),
	GroupID: "inspection",
	RunE:    handleValidate,
}

func init() {
	RootCmd.AddCommand(validateCmd)
}
