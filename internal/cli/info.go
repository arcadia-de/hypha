package cli

import (
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func handleInfo(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	orc.PrintRuntimeInfo()
	return nil
}

var infoCmd = &cobra.Command{
	Use:   "info",
	Short: "Show system info",
	RunE:  handleInfo,
}

func init() {
	RootCmd.AddCommand(infoCmd)
}
