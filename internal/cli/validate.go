package cli

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"

	"github.com/arcadia-de/hypha/internal/hypha"
)

func handleValidate(cmd *cobra.Command, args []string) error {
	style := NewValidateStyle()
	validator, err := hypha.NewSchemaValidator()
	if err != nil {
		style.PrintError(err)
		return nil
	}

	fmt.Println()
	results := validator.ValidateSchemas(args)
	summary := style.PrintResults(results)
	style.PrintSummary(summary)
	if summary.HasInvalid() {
		os.Exit(1)
	}

	return nil
}

func init() {
	validateCmd := &cobra.Command{
		Use:     "validate manifests...",
		Short:   "Validate the specified manifests",
		GroupID: "inspection",
		Args:    cobra.MinimumNArgs(1),
		RunE:    handleValidate,
	}
	AddFormatFlags(validateCmd)

	RootCmd.AddCommand(validateCmd)
}
