package cli

import (
	"charm.land/log/v2"
	"fmt"
	"github.com/spf13/cobra"
)

func handleDescribe(kind string, args []string) error {
	log.Info("describe not implemented", "kind", kind, "id", args[0])
	return nil
}

func createDescribeResourceCommand(kind string) *cobra.Command {
	return &cobra.Command{
		Use: kind + "s id",
		Aliases: []string{
			kind,
		},
		Short: fmt.Sprintf("Describe %s resources", kind),
		Args:  cobra.MinimumNArgs(1),
		RunE: func(cmd *cobra.Command, args []string) error {
			return handleDescribe(kind, args)
		},
	}
}

func CreateDescribeCommand(kinds []string) *cobra.Command {
	describeCmd := &cobra.Command{
		Use: "describe",
		Aliases: []string{
			"desc",
		},
		Short:   "Describe a resource",
		GroupID: "inspection",
	}

	for _, kind := range kinds {
		describeCmd.AddCommand(createDescribeResourceCommand(kind))
	}

	return describeCmd
}
