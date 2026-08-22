package cli

import (
	"fmt"
	"github.com/spf13/cobra"
)

func HandleDocs(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("docs is not implemented")
}

func init() {
	docsCmd := &cobra.Command{
		Use:     "docs [kind]",
		Short:   "Open the documentation for a specific resource kind in the system browser",
		Args:    cobra.ExactArgs(1),
		GroupID: "development",
		RunE:    HandleDocs,
	}

	RootCmd.AddCommand(docsCmd)
}
