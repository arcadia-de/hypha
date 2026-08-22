package cli

import (
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func HandleInfo(cmd *cobra.Command, args []string) error {
	//TODO(@s0cks): implement
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()
	orc.ProcessDiscoveredManifests()
	orc.PrintRuntimeInfo()
	return nil
}

func init() {
	infoCmd := &cobra.Command{
		Use:   "info",
		Short: "Show runtime info",
		RunE:  HandleInfo,
	}

	RootCmd.AddCommand(infoCmd)
}
