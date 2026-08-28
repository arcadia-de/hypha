package cli

import (
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

func HandleBrowse(cmd *cobra.Command, args []string) error {
	if web := viper.GetBool("web"); web {
		return HandleBrowseWeb()
	}

	return HandleBrowseTui()
}

func init() {
	browseCmd := &cobra.Command{
		Use:     "browse",
		Short:   "Open a read-only interactive browser session",
		GroupID: "inspection",
		RunE:    HandleBrowse,
	}
	browseCmd.Flags().BoolP("web", "w", false, "Serve the web dashboard")
	browseCmd.Flags().BoolP("open", "", true, "Open the web dashboard")

	RootCmd.AddCommand(browseCmd)
}
