package cli

import (
	"github.com/spf13/cobra"

	"charm.land/log/v2"
)

func handleLsp(cmd *cobra.Command, args []string) error {
	log.Info("lsp is not implemented")
	return nil
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
