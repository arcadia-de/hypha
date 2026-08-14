package cli

import (
	"charm.land/log/v2"
	"github.com/spf13/cobra"
)

func handleTidy(cmd *cobra.Command, args []string) error {
	log.Info("tidy is not implemented")
	return nil
}

var tidyCmd = &cobra.Command{
	Use:     "tidy",
	Short:   "Cleanup the configuration dir",
	GroupID: "config",
	RunE:    handleTidy,
}

func init() {
	RootCmd.AddCommand(tidyCmd)
}
