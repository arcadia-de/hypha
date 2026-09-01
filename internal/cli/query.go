package cli

import (
	"fmt"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

var queryExpr string
var queryFile string

func HandleQuery(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	info := hypha.RunInfo{
		Mode: hypha.RunObserve,
	}

	if err := orc.Run(info); err != nil {
		return fmt.Errorf("failed to run orchestrator: %v", err)
	}

	var results any = nil
	if queryExpr != "" {
		results, err = orc.Query(queryExpr)
		if err != nil {
			HandleQueryFailed(queryExpr, err)
			return nil
		}

	} else if queryFile != "" {
		// TODO(@s0cks): implement
		HandleQueryFailed(queryExpr, fmt.Errorf("not implemented"))
		return nil
	}

	reporter := NewQueryResultReporter()
	reporter.Handle(queryExpr, results)

	return nil
}

func init() {
	queryCmd := &cobra.Command{
		Use:     "query",
		Short:   "Query the resource graph using an expression",
		RunE:    HandleQuery,
		GroupID: "inspection",
	}

	queryCmd.Flags().StringVarP(&queryFile, "file", "f", "", "The query file to evaluate")
	queryCmd.Flags().StringVarP(&queryExpr, "expr", "e", "", "The query expression to evaluate")
	queryCmd.MarkFlagsOneRequired("file", "expr")
	queryCmd.MarkFlagsMutuallyExclusive("file", "expr")
	queryCmd.Flags().StringP("format", "", "pretty", "The output format. Values: [ json, yaml, plain, colored, pretty* ]")

	RootCmd.AddCommand(queryCmd)
}
