package cli

import (
	"fmt"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func handleRocks(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	luarocks, err := hypha.GetPackageManager("luarocks")
	if err != nil {
		return err
	}

	for _, pkg := range args {
		fmt.Printf("installing %s\n", pkg)
		luarocks.Install(pkg)
	}

	return nil
}

func init() {
	rocksCmd := &cobra.Command{
		Use:   "rocks [package...]",
		Short: "Install a luarocks package",
		Args:  cobra.MinimumNArgs(1),
		RunE:  handleRocks,
	}

	RootCmd.AddCommand(rocksCmd)
}
