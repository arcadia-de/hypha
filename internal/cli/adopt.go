package cli

import (
	"charm.land/log/v2"
	"github.com/spf13/cobra"
)

func handleAdopt(cmd *cobra.Command, args []string) error {
	log.Info("adopt is not implemented")
	return nil
}

var adoptCmd = &cobra.Command{
	Use:     "adopt [resources]",
	Short:   "Adopt specific resources into the resource graph",
	Args:    cobra.MinimumNArgs(1),
	GroupID: "config",
	RunE:    handleAdopt,
}

func init() {
	RootCmd.AddCommand(adoptCmd)
}
