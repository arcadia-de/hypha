package cli

import (
	"fmt"
	"github.com/spf13/cobra"
)

func HandleStatus(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("status is not implemented")
}

func init() {
	statusCommand := &cobra.Command{
		Use:     "status",
		Short:   "Show resource drift",
		GroupID: "config",
		RunE:    HandleStatus,
	}

	RootCmd.AddCommand(statusCommand)
}
