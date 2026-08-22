package cli

import (
	"fmt"
	"github.com/spf13/cobra"
)

func HandleExplain(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("explain is not implemented")
}

func init() {
	explainCmd := &cobra.Command{
		Use:     "explain [id]",
		Short:   "Explain why a resource exists",
		Args:    cobra.ExactArgs(1),
		GroupID: "inspection",
		RunE:    HandleExplain,
	}

	RootCmd.AddCommand(explainCmd)
}
