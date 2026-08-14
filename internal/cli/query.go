package cli

import (
	"fmt"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

var queryExpr string
var queryFile string

func handleQuery(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	if queryExpr != "" {
		results, err := orc.Query(queryExpr)
		if err != nil {
			return fmt.Errorf("query expr '%s' failed: %v", queryExpr, err)
		}

		fmt.Printf("results:\n%s\n", results)
	} else if queryFile != "" {
		// TODO(@s0cks): implement
		return fmt.Errorf("not implemented")
	}

	return nil
}

var queryCmd = &cobra.Command{
	Use:     "query",
	Short:   "Query the resource graph using an expression",
	RunE:    handleQuery,
	GroupID: "inspection",
}

func init() {
	queryCmd.Flags().StringVarP(&queryFile, "file", "f", "", "The query file to evaluate")
	queryCmd.Flags().StringVarP(&queryExpr, "expr", "e", "", "The query expression to evaluate")
	queryCmd.MarkFlagsOneRequired("file", "expr")
	queryCmd.MarkFlagsMutuallyExclusive("file", "expr")
	RootCmd.AddCommand(queryCmd)
}
