package cli

import (
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func handleGraph(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}

	filename := args[0]
	specs, err := orc.ParseResourceSpecsFromJsonnet(filename)
	if err != nil {
		return fmt.Errorf("failed to parse resource specs from %s: %v", filename, err)
	}

	for i := range specs {
		orc.AddResource(specs[i])
	}

	err = orc.RenderGraph("test", "dot", "plain")
	if err != nil {
		return fmt.Errorf("failed to render resource graph: %v", err)
	}

	return nil
}

var graphCmd = &cobra.Command{
	Use:     "graph [manifest]",
	Short:   "Graph the resources",
	Args:    cobra.ExactArgs(1),
	GroupID: "inspection",
	RunE:    handleGraph,
}

func init() {
	RootCmd.AddCommand(graphCmd)
}
