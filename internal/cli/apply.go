package cli

import (
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func handleApply(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}

	filename := args[0]
	specs, err := orc.ParseResourceSpecsFromJsonnet(filename)
	if err != nil {
		return fmt.Errorf("failed to parse resource specs from %s: %v", filename, err)
	}

	for i := range specs {
		orc.AddResource(specs[i])
	}

	err = orc.Run()
	if err != nil {
		return fmt.Errorf("failed to run Orchestrator: %v", err)
	}

	return nil
}

var applyCmd = &cobra.Command{
	Use:     "apply [manifest]",
	Short:   "Apply a manifest",
	Args:    cobra.ExactArgs(1),
	GroupID: "config",
	RunE:    handleApply,
}

func init() {
	RootCmd.AddCommand(applyCmd)
}
