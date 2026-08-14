package cli

import (
	"charm.land/log/v2"
	"github.com/spf13/cobra"
)

func handleLint(cmd *cobra.Command, args []string) error {
	log.Info("lint is not implemented")
	return nil
}

var lintCmd = &cobra.Command{
	Use:     "lint [manifests]",
	Args:    cobra.MinimumNArgs(1),
	Short:   "Lint the specified manifests",
	GroupID: "inspection",
	RunE:    handleLint,
}

func init() {
	RootCmd.AddCommand(lintCmd)
}
