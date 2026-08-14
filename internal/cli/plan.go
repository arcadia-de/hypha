package cli

import (
	"github.com/spf13/cobra"

	"charm.land/log/v2"
)

func handlePlan(cmd *cobra.Command, args []string) error {
	log.Info("plan is not implemented")
	return nil
}

var planCmd = &cobra.Command{
	Use:     "plan",
	Short:   "Preview the pending changes",
	GroupID: "config",
	RunE:    handlePlan,
}

func init() {
	RootCmd.AddCommand(planCmd)
}
