package cli

import (
	"charm.land/log/v2"
	"github.com/spf13/cobra"
)

func handleStatus(cmd *cobra.Command, args []string) error {
	log.Info("status is not implemented")
	return nil
}

var statusCommand = &cobra.Command{
	Use:     "status",
	Short:   "Show resource drift",
	GroupID: "config",
	RunE:    handleStatus,
}

func init() {
	RootCmd.AddCommand(statusCommand)
}
