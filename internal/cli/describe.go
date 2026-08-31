package cli

import (
	"fmt"
	"strings"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func CreateDescribeFilter(kind string, refs []string) hypha.ResourceSelector {
	filters := []hypha.ResourceSelector{}
	filters = append(filters, hypha.NewKindResourceSelector(kind))
	if len(refs) > 0 {
		filters = append(filters, hypha.NewRefsResourceSelector(refs))
	}

	return hypha.NewAndResourceSelector(filters)
}

func HandleDescribeKind(kind string, args []string) error {
	_ = kind

	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	info := hypha.RunInfo{
		Mode: hypha.RunObserve,
	}

	if err := orc.ProcessDiscoveredManifests(info.Mode); err != nil {
		return fmt.Errorf("failed to process discovered manifests: %v", err)
	}
	if err := orc.Run(info); err != nil {
		return fmt.Errorf("failed to run orchestrator: %v", err)
	}

	filter := CreateDescribeFilter(kind, args)
	defer filter.Close()

	rg := orc.GetResourceGraph()
	resources := rg.ListResourcesWithSelector(filter)
	reporter := GetDescribeReporter()
	err = reporter.Run(resources)
	if err != nil {
		return fmt.Errorf("failed to report resources")
	}

	return nil
}

func CreateDescribeKindCommand(kind string) *cobra.Command {
	cmd := &cobra.Command{
		Use: kind + "s",
		Aliases: []string{
			kind,
			strings.ToLower(kind),
			strings.ToLower(kind) + "s",
		},
		Short: fmt.Sprintf("Describe %s resources", kind),
		RunE: func(cmd *cobra.Command, args []string) error {
			return HandleDescribeKind(kind, args)
		},
	}
	cmd.PersistentFlags().StringP("format", "f", "pretty", fmt.Sprintf("Change the output format. Valid values are: %s", strings.Join(GetListReporters(), ", ")))

	return cmd
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
		describeCmd.AddCommand(CreateDescribeKindCommand(kind))
	}

	return describeCmd
}
