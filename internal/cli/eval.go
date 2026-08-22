package cli

import (
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

var expr string
var file string

func HandleEval(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	err = orc.Run(hypha.OrchestratorApplyMode) // TODO(@s0cks): should have a noop mode
	if err != nil {
		return fmt.Errorf("failed to run Orchestrator: %v", err)
	}

	if expr != "" {
		orc.EvalLuaExpr(expr)
	} else if file != "" {
		orc.EvalLuaFile(file)
	}

	return nil
}

func init() {
	evalCmd := &cobra.Command{
		Use:     "eval",
		Short:   "Evaluate a lua expression or file",
		GroupID: "development",
		RunE:    HandleEval,
	}
	evalCmd.Flags().StringVarP(&file, "file", "f", "", "The lua file to evaluate")
	evalCmd.Flags().StringVarP(&expr, "expr", "e", "", "The lua expression to evaluate")
	evalCmd.MarkFlagsOneRequired("file", "expr")
	evalCmd.MarkFlagsMutuallyExclusive("file", "expr")

	RootCmd.AddCommand(evalCmd)
}
