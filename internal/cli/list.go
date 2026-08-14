package cli

import (
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

func createListFilter() hypha.ResourceSelector {
	var filter hypha.ResourceSelector
	listKindFilter := viper.GetString("kind")
	if listKindFilter != "" {
		filter = hypha.NewKindResourceSelector(listKindFilter)
	}

	listLabelFilter := viper.GetString("label")
	if listLabelFilter != "" {
		if filter.Handle == nil {
			filter = hypha.NewLabelResourceSelector(listLabelFilter)
		}
	}

	return filter
}

func handleList(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	filter := createListFilter()
	defer filter.Close()

	filename := args[0]
	specs, err := orc.ParseResourceSpecsFromJsonnet(filename)
	if err != nil {
		return fmt.Errorf("failed to parse resource specs from %s: %v", filename, err)
	}

	for i := range specs {
		orc.AddResource(specs[i])
	}

	var records []hypha.Resource
	if filter.Handle != nil {
		orc.VisitAllMatchingResources(filter, func(rec hypha.Resource) bool {
			records = append(records, rec)
			return true
		})
	} else {
		orc.VisitAllResources(func(rec hypha.Resource) bool {
			records = append(records, rec)
			return true
		})
	}

	for _, r := range records {
		fmt.Printf(" - %s (%s)\n", r.ID, r.Kind)
	}

	return nil
}

func init() {
	listCmd := &cobra.Command{
		Use:   "list [manifest]",
		Short: "List resources matching a specific selector",
		Args:  cobra.ExactArgs(1),
		RunE:  handleList,
	}

	listCmd.Flags().StringP("kind", "k", "", "Filter by kind")
	listCmd.Flags().StringP("label", "l", "", "Filter by label")

	RootCmd.AddCommand(listCmd)
}
