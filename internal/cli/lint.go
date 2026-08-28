package cli

import (
	"fmt"
	"github.com/spf13/cobra"
)

func HandleLint(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("lint not implemented")
}

func init() {
	lintCmd := &cobra.Command{
		Use:     "lint [manifests]",
		Short:   "Lint the specified manifests",
		GroupID: "inspection",
		RunE:    HandleLint,
	}

	RootCmd.AddCommand(lintCmd)
}
