package cli

import (
	"charm.land/log/v2"
	"github.com/spf13/cobra"
)

func handleDocs(cmd *cobra.Command, args []string) error {
	log.Info("docs is not implemented")
	return nil
}

var docsCmd = &cobra.Command{
	Use:     "docs [kind]",
	Short:   "Open the documentation for a specific resource kind in the system browser",
	Args:    cobra.ExactArgs(1),
	GroupID: "development",
	RunE:    handleDocs,
}

func init() {
	RootCmd.AddCommand(docsCmd)
}
