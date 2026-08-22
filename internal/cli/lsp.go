package cli

import (
	"fmt"
	"github.com/spf13/cobra"
)

func handleLsp(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("lsp is not implemented")
}

func init() {
	lspCmd := &cobra.Command{
		Use:     "lsp",
		Short:   "Run the LSP service for a manifest",
		GroupID: "development",
		RunE:    handleLsp,
	}

	RootCmd.AddCommand(lspCmd)
}
