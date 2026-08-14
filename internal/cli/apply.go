package cli

import (
	"encoding/json"
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

func handleApply(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	specs, err := orc.ParseResourceSpecsFromJsonnet(args[0])
	if err != nil {
		return fmt.Errorf("failed to parse resource specs from %s: %v", args[0], err)
	}

	for _, s := range specs {
		orc.AddResource(s)
	}

	err = orc.Run(hypha.OrchestratorApplyMode)
	if err != nil {
		return fmt.Errorf("failed to run Orchestrator: %v", err)
	}

	telemetry := viper.GetBool("print-telemetry")
	if telemetry {
		metrics := orc.GetMetrics()
		fmt.Println("Telemetry:")
		s, err := json.Marshal(metrics)
		if err != nil {
			return fmt.Errorf("failed to marshal OrchestratorMetrics: %v", err)
		}
		fmt.Println(string(s))
	}

	return nil
}

func init() {
	applyCmd := &cobra.Command{
		Use:     "apply [manifest]",
		Short:   "Apply a manifest",
		Args:    cobra.ExactArgs(1),
		GroupID: "config",
		RunE:    handleApply,
	}

	applyCmd.Flags().BoolP("print-telemetry", "", false, "Enable telemetry")

	RootCmd.AddCommand(applyCmd)
}
