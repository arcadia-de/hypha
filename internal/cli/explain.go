package cli

import (
	"charm.land/log/v2"
	"github.com/spf13/cobra"
)

func handleExplain(cmd *cobra.Command, args []string) error {
	log.Info("explain is not implemented")
	return nil
}

var explainCmd = &cobra.Command{
	Use:     "explain [id]",
	Short:   "Explain why a resource exists",
	Args:    cobra.ExactArgs(1),
	GroupID: "inspection",
	RunE:    handleExplain,
}

func init() {
	RootCmd.AddCommand(explainCmd)
}
