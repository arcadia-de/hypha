package cli

import (
	"fmt"
	"github.com/spf13/cobra"
)

func handleGraph(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("graph command not implemented")
}

func init() {
	graphCmd := &cobra.Command{
		Use:     "graph",
		Short:   "Graph the resources",
		GroupID: "inspection",
		RunE:    handleGraph,
	}

	RootCmd.AddCommand(graphCmd)
}
