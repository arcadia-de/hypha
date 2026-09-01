package cli

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
	"github.com/spf13/cobra/doc"
)

func HandleGenTldrDocs(cmd *cobra.Command, args []string) error {
	return GenTldrPage(os.Stdout)
}

func HandleGenManDocs(cmd *cobra.Command, args []string) error {
	outDir := os.Getenv("MESON_INSTALL_DESTDIR_MAN")
	if outDir == "" {
		outDir = "./dist/man" // fallback
	}
	_ = os.MkdirAll(outDir, 0755)

	header := &doc.GenManHeader{
		Title:   "Hypha",
		Section: "1",
	}

	if err := doc.GenManTree(RootCmd, header, outDir); err != nil {
		return fmt.Errorf("failed to generate man docs: %v", err)
	}

	return nil
}

func init() {
	gendocs := &cobra.Command{
		Use:    "gen-docs",
		Hidden: true,
	}

	genmandocs := &cobra.Command{
		Use:  "man",
		RunE: HandleGenManDocs,
	}
	gendocs.AddCommand(genmandocs)

	gentldrdocs := &cobra.Command{
		Use:  "tldr",
		RunE: HandleGenTldrDocs,
	}
	gendocs.AddCommand(gentldrdocs)

	RootCmd.AddCommand(gendocs)
}
