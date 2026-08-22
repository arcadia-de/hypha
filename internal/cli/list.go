package cli

import (
	"fmt"
	"os"
	"strings"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

func CreateListFilter(kind string) hypha.ResourceSelector {
	var filters []hypha.ResourceSelector

	kindFilter := hypha.NewKindResourceSelector(kind)
	filters = append(filters, kindFilter)

	labelFilter := viper.GetString("label")
	if labelFilter != "" {
		filters = append(filters, hypha.NewLabelResourceSelector(labelFilter))
	}

	return hypha.NewAndResourceSelector(filters)
}

func HandleListResourcesByKindCommand(kind string, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	orc.ProcessDiscoveredManifests()

	rg := orc.GetResourceGraph()
	if !rg.IsEmpty() {
		filter := CreateListFilter(kind)
		defer filter.Close()

		reporter := GetListReporter()
		resources := rg.ListResourcesWithOptionalSelector(filter)
		if _, err := reporter.Run(resources); err != nil {
			fmt.Printf("Error running program: %v\n", err)
			os.Exit(1)
		}
	}

	return nil
}

func CreateListResourcesByKindCommand(kind string) *cobra.Command {
	listKindCommand := &cobra.Command{
		Use: kind + "s",
		Aliases: []string{
			kind,
			strings.ToLower(kind),
			strings.ToLower(kind + "s"),
		},
		Short: fmt.Sprintf("List %s resource", kind),
		RunE: func(cmd *cobra.Command, args []string) error {
			return HandleListResourcesByKindCommand(kind, args)
		},
	}
	listKindCommand.Flags().StringP("label", "l", "", "Filter by label")
	return listKindCommand
}

func GetListReporters() []string {
	return []string{
		"json",
		"jsonl",
		"yaml",
		"plain",
		"colored",
		"pretty",
	}
}

func HandleDefaultListResources(args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	orc.ProcessDiscoveredManifests()

	rg := orc.GetResourceGraph()
	if !rg.IsEmpty() {
		reporter := GetListReporter()
		resources := rg.ListResources()
		if _, err := reporter.Run(resources); err != nil {
			fmt.Printf("Error running program: %v\n", err)
			os.Exit(1)
		}
	}

	return nil
}

func CreateListCommand(kinds []string) *cobra.Command {
	listCmd := &cobra.Command{
		Use:   "list",
		Short: "List resources in the graph",
		Aliases: []string{
			"ls",
		},
		GroupID: "inspection",
		RunE: func(cmd *cobra.Command, args []string) error {
			return HandleDefaultListResources(args)
		},
	}
	listCmd.PersistentFlags().StringP("format", "f", "pretty", fmt.Sprintf("Change the output format. Valid values are: %s", strings.Join(GetListReporters(), ", ")))

	for _, kind := range kinds {
		listCmd.AddCommand(CreateListResourcesByKindCommand(kind))
	}

	return listCmd
}
