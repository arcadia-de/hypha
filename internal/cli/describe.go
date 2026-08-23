package cli

import (
	"fmt"
	"strings"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func CreateDescribeFilter(kind string, refs []string) hypha.ResourceSelector {
	kindFilter := hypha.NewKindResourceSelector(kind)
	refsFilter := hypha.NewRefsResourceSelector(refs)
	return hypha.NewAndResourceSelector([]hypha.ResourceSelector{
		kindFilter,
		refsFilter,
	})
}

func handleDescribe(kind string, args []string) error {
	_ = kind

	rowStyle := lg.NewStyle().MarginLeft(2)
	style := NewDescribeStyle(&rowStyle)

	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	orc.ProcessDiscoveredManifests()

	fmt.Println()

	filter := CreateDescribeFilter(kind, args)
	defer filter.Close()

	rg := orc.GetResourceGraph()
	resources := rg.ListResourcesWithSelector(filter)
	for _, r := range resources {
		fmt.Println()
		style.Print(r)
	}
	fmt.Println()

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
