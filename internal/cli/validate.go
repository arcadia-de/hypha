package cli

import (
	"fmt"
	"os"

	lg "charm.land/lipgloss/v2"
	"github.com/spf13/cobra"

	"github.com/arcadia-de/hypha/internal/hypha"
	schema "github.com/arcadia-de/hypha/schema"
)

func handleValidate(cmd *cobra.Command, args []string) error {
	validator, err := hypha.NewSchemaValidator()
	if err != nil {
		fmt.Printf("schema is not valid\n")
		fmt.Printf("schema:\n")
		fmt.Printf("%s\n", string(schema.ManifestSchemaJson))
		fmt.Printf("error:\n%v\n", err)
		return nil
	}

	const (
		validInd   = ""
		invalidInd = ""
	)

	indStyle := lg.NewStyle().Bold(true).Foreground(lg.Color("#FFFFFF"))
	validStyle := lg.NewStyle().Bold(true).Foreground(lg.Color("#d4edda"))
	invalidStyle := lg.NewStyle().Bold(true).Foreground(lg.Color("#f8d7da"))

	vm := hypha.CreateJsonnetVM()

	invalidCount := 0

	fmt.Println()
	results := validator.ValidateSchemas(vm, args)
	for i, result := range results {
		if result.IsValid {
			fmt.Println(validStyle.Render(fmt.Sprintf("  %s %s", validInd, result.Filename)))
		} else {
			fmt.Println(invalidStyle.Render(fmt.Sprintf("  %s %s", invalidInd, result.Filename)))
			fmt.Printf("  Error: %s\n", indStyle.Render(result.Error.Error()))
			invalidCount++
			if i < (len(results) - 1) {
				fmt.Println()
			}
		}
	}
	fmt.Println()

	if invalidCount == len(results) {
		fmt.Println(invalidStyle.Render(fmt.Sprintf("  %s All manifests are invalid", invalidInd)))
	} else {
		if invalidCount != 0 {
			fmt.Println(validStyle.Render(fmt.Sprintf("  %s %d/%d Valid", validInd, (len(results) - invalidCount), len(results))))
			fmt.Println(invalidStyle.Render(fmt.Sprintf("  %s %d/%d Invalid", invalidInd, invalidCount, len(results))))
		} else {
			fmt.Println(validStyle.Render(fmt.Sprintf("  %s %s", validInd, "All manifests are valid")))
		}
	}

	if invalidCount > 0 {
		os.Exit(1)
	}

	return nil
}

func init() {
	validateCmd := &cobra.Command{
		Use:     "validate [manifests]",
		Short:   "Validate the specified manifests",
		GroupID: "inspection",
		RunE:    handleValidate,
	}

	RootCmd.AddCommand(validateCmd)
}
