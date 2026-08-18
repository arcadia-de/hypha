package cli

import (
	"fmt"
	"strings"

	lg "charm.land/lipgloss/v2"
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

func PrintResourceList(resources []hypha.Resource) {
	fmt.Println()

	const (
		rowPadding = 2

		stateColWidth = 10
		idColWidth    = 25
		nameColWidth  = 50
		numCols       = 3

		totalSize = rowPadding + stateColWidth + idColWidth + nameColWidth + numCols
	)

	rowStyle := lg.NewStyle().
		PaddingLeft(rowPadding)

	baseStateStyle := lg.NewStyle().
		Width(stateColWidth).
		Align(lg.Right)

	pendingStyle := baseStateStyle.
		Foreground(lg.Color("#8B7EC8"))

	processingStyle := baseStateStyle.
		Foreground(lg.Color("#4385BE"))

	readyStyle := baseStateStyle.
		Foreground(lg.Color("#879A39"))

	failedStyle := baseStateStyle.
		Foreground(lg.Color("#D14D41"))

	unknownStyle := baseStateStyle.
		Foreground(lg.Color("9F9D96"))

	idStyle := lg.NewStyle().
		Width(idColWidth).
		Foreground(lg.Color("#575653")).
		Align(lg.Center)

	nameStyle := lg.NewStyle().
		Width(nameColWidth).
		Foreground(lg.Color("#CECDC3")).
		Align(lg.Left)

	separator := strings.Repeat("-", totalSize)
	separatorStyle := lg.NewStyle().
		Align(lg.Center).
		Foreground(lg.Color("#282726"))

	headerStyle := lg.NewStyle().
		Foreground(lg.Color("#575653"))

	headerStateStyle := headerStyle.
		Width(stateColWidth).
		Align(lg.Right)

	headerIdStyle := headerStyle.
		Width(idColWidth).
		Align(lg.Center)

	headerNameStyle := headerStyle.
		Width(nameColWidth).
		Align(lg.Left)

	header := fmt.Sprintf("%s %s %s", headerStateStyle.Render("State"), headerIdStyle.Render("ID"), headerNameStyle.Render("Name"))
	fmt.Println(rowStyle.Render(header))
	fmt.Println(rowStyle.Render(separatorStyle.Render(separator)))

	for _, resource := range resources {
		var stateStyle lg.Style
		switch resource.State {
		case "Pending":
			stateStyle = pendingStyle
		case "Processing":
			stateStyle = processingStyle
		case "Ready":
			stateStyle = readyStyle
		case "Failed":
			stateStyle = failedStyle
		case "Unknown":
			stateStyle = unknownStyle
		default:
			stateStyle = unknownStyle
		}

		state := stateStyle.Render(resource.State)

		id := idStyle.Render(resource.ID)

		name := nameStyle.Render("Name")

		fmt.Println(rowStyle.Render(fmt.Sprintf("%s %s %s", state, id, name)))
	}

	fmt.Println()
}

func HandleListResourcesByKindCommand(kind string, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	orc.ProcessDiscoveredManifests()

	filter := CreateListFilter(kind)
	defer filter.Close()

	resources := orc.ListResourcesWithOptionalSelector(filter)
	PrintResourceList(resources)

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

func CreateListCommand(kinds []string) *cobra.Command {
	listCmd := &cobra.Command{
		Use:   "list",
		Short: "List resources in the graph",
		Aliases: []string{
			"ls",
		},
		GroupID: "inspection",
	}

	for _, kind := range kinds {
		listCmd.AddCommand(CreateListResourcesByKindCommand(kind))
	}

	return listCmd
}
