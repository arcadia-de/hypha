package cli

import (
	"charm.land/log/v2"
	"github.com/spf13/cobra"
)

func handleBrowse(cmd *cobra.Command, args []string) error {
	log.Info("browse is not implemented")
	return nil
}

var browseCmd = &cobra.Command{
	Use:     "browse",
	Short:   "Open a read-only interactive browser session",
	GroupID: "inspection",
	RunE:    handleBrowse,
}

func init() {
	RootCmd.AddCommand(browseCmd)
}
