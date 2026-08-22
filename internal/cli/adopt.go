package cli

import (
	"fmt"
	"github.com/spf13/cobra"
)

func HandleAdopt(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("adopt is not implemented")
}

func init() {
	adoptCmd := &cobra.Command{
		Use:     "adopt [resources]",
		Short:   "Adopt specific resources into the resource graph",
		Args:    cobra.MinimumNArgs(1),
		GroupID: "config",
		RunE:    HandleAdopt,
	}

	RootCmd.AddCommand(adoptCmd)
}
