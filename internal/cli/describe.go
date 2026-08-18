package cli

import (
	"fmt"
	"strings"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func createDescribeFilter(ids []string) hypha.ResourceSelector {
	var filter hypha.ResourceSelector

	if len(ids) > 0 {
		var filters []hypha.ResourceSelector
		for _, id := range ids {
			filters = append(filters, hypha.NewIdFilter(id))
		}

		filter = hypha.NewOrResourceSelector(filters)
	}

	return filter
}

func handleDescribe(kind string, args []string) error {
	_ = kind

	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	orc.ProcessDiscoveredManifests()

	fmt.Println()

	filter := createDescribeFilter(args)
	defer filter.Close()

	var records []hypha.Resource
	orc.VisitAllMatchingResources(filter, func(rec hypha.Resource) bool {
		records = append(records, rec)
		return true
	})

	for _, r := range records {
		fmt.Printf(" - %s (%s)\n", r.ID, r.Kind)
	}

	return nil
}

func createDescribeResourceCommand(kind string) *cobra.Command {
	return &cobra.Command{
		Use: kind + "s",
		Aliases: []string{
			kind,
			strings.ToLower(kind),
			strings.ToLower(kind) + "s",
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
