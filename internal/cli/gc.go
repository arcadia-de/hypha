package cli

import (
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func handleGc(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	err = orc.CollectGarbage()
	if err != nil {
		return fmt.Errorf("failed to gc: %v", err)
	}

	return nil
}

var gcCmd = &cobra.Command{
	Use:     "gc",
	Short:   "Cleanup orphaned resources",
	GroupID: "config",
	RunE:    handleGc,
}

func init() {
	RootCmd.AddCommand(gcCmd)
}
