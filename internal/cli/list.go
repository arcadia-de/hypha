package cli

import (
	"encoding/json"
	"fmt"
	"os"
	"strings"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
	"gopkg.in/yaml.v3"
)

type Theme struct {
	RowStyle lg.Style

	PendingStyle    lg.Style
	ProcessingStyle lg.Style
	ReadyStyle      lg.Style
	FailedStyle     lg.Style
	UnknownStyle    lg.Style

	NameStyle lg.Style
	IdStyle   lg.Style

	HeaderStyle      lg.Style
	HeaderStateStyle lg.Style
	HeaderIdStyle    lg.Style
	HeaderNameStyle  lg.Style
}

func (th Theme) GetStateStyle(state string) lg.Style {
	var stateStyle lg.Style
	switch state {
	case "Pending":
		stateStyle = th.PendingStyle
	case "Processing":
		stateStyle = th.ProcessingStyle
	case "Ready":
		stateStyle = th.ReadyStyle
	case "Failed":
		stateStyle = th.FailedStyle
	case "Unknown":
		stateStyle = th.UnknownStyle
	default:
		stateStyle = th.UnknownStyle
	}
	return stateStyle
}

func CreateBaseTheme() Theme {
	const (
		stateColWidth = 14
		idColWidth    = 25
		nameColWidth  = 25
		numCols       = 3
	)

	baseStateStyle := lg.NewStyle().
		Width(stateColWidth).
		Align(lg.Right)

	headerStyle := lg.NewStyle()
	return Theme{
		RowStyle: lg.NewStyle(),
		HeaderStyle: lg.NewStyle().
			BorderStyle(lg.NormalBorder()).
			BorderBottom(true),

		IdStyle: lg.NewStyle().
			Width(idColWidth).
			Align(lg.Center),

		NameStyle: lg.NewStyle().
			Width(nameColWidth).
			Align(lg.Left),

		PendingStyle:    baseStateStyle,
		ProcessingStyle: baseStateStyle,
		ReadyStyle:      baseStateStyle,
		FailedStyle:     baseStateStyle,
		UnknownStyle:    baseStateStyle,

		HeaderStateStyle: headerStyle.
			Width(stateColWidth).
			Align(lg.Right),
		HeaderIdStyle: headerStyle.
			Width(idColWidth).
			Align(lg.Center),
		HeaderNameStyle: headerStyle.
			Width(nameColWidth).
			Align(lg.Left),
	}
}

func CreateColoredTheme() Theme {
	base := CreateBaseTheme()
	return Theme{
		RowStyle: base.RowStyle,

		IdStyle: base.IdStyle.
			Foreground(lg.Color("#575653")),

		NameStyle: base.NameStyle.
			Foreground(lg.Color("#CECDC3")),

		PendingStyle: base.PendingStyle.
			Foreground(lg.Color("#8B7EC8")),
		ProcessingStyle: base.PendingStyle.
			Foreground(lg.Color("#4385BE")),
		ReadyStyle: base.PendingStyle.
			Foreground(lg.Color("#879A39")),
		FailedStyle: base.PendingStyle.
			Foreground(lg.Color("#D14D41")),
		UnknownStyle: base.PendingStyle.
			Foreground(lg.Color("9F9D96")),

		HeaderStyle: base.HeaderStyle.
			BorderForeground(lg.Color("#282726")),
		HeaderStateStyle: base.HeaderStateStyle.
			Foreground(lg.Color("#575653")),
		HeaderNameStyle: base.HeaderNameStyle.
			Foreground(lg.Color("#575653")),
		HeaderIdStyle: base.HeaderIdStyle.
			Foreground(lg.Color("#575653")),
	}
}

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

func PrintResourceList(theme Theme, resources []hypha.Resource) {
	fmt.Println()

	headerRow := lg.JoinHorizontal(
		lg.Left,
		theme.HeaderStateStyle.Render("State"),
		theme.HeaderIdStyle.Render("ID"),
		theme.HeaderNameStyle.Render("Name"),
	)
	fmt.Println(theme.HeaderStyle.Render(headerRow))

	for _, resource := range resources {
		stateStyle := theme.GetStateStyle(resource.State)
		row := lg.JoinHorizontal(
			lg.Left,
			stateStyle.Render(resource.State),
			theme.IdStyle.Render(resource.ID),
			theme.NameStyle.Render("Name"),
		)
		fmt.Println(theme.RowStyle.Render(row))
	}

	fmt.Println()
}

type ListProgram interface {
	Run(resources []hypha.Resource) (any, error)
}

type NerdfontListProgram struct{}

func (p NerdfontListProgram) Run(resources []hypha.Resource) (any, error) {
	theme := CreateColoredTheme()
	PrintResourceList(theme, resources)
	return nil, nil
}

type ColoredListProgram struct{}

func (p ColoredListProgram) Run(resources []hypha.Resource) (any, error) {
	theme := CreateColoredTheme()
	PrintResourceList(theme, resources)
	return nil, nil
}

type JsonListProgram struct{}

func (p JsonListProgram) Run(resources []hypha.Resource) (any, error) {
	bytes, err := json.MarshalIndent(resources, "", "  ")
	if err != nil {
		return nil, err
	}

	fmt.Println(string(bytes))
	return nil, nil
}

type JsonLinesListProgram struct{}

func (p JsonLinesListProgram) Run(resources []hypha.Resource) (any, error) {
	for _, res := range resources {
		bytes, err := json.Marshal(res)
		if err != nil {
			return nil, err
		}

		fmt.Println(string(bytes))
	}

	return nil, nil
}

type YamlListProgram struct{}

func (p YamlListProgram) Run(resources []hypha.Resource) (any, error) {
	bytes, err := yaml.Marshal(resources)
	if err != nil {
		return nil, err
	}

	fmt.Println(string(bytes))
	return nil, nil
}

type PlainListProgram struct{}

func (p PlainListProgram) Run(resources []hypha.Resource) (any, error) {
	theme := CreateBaseTheme()
	PrintResourceList(theme, resources)
	return nil, nil
}

func HandleListResourcesByKindCommand(kind string, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	var program ListProgram
	format := viper.GetString("format")
	switch format {
	case "json":
		program = JsonListProgram{}
	case "jsonl":
		program = JsonLinesListProgram{}
	case "yaml":
		program = YamlListProgram{}
	case "plain":
		program = PlainListProgram{}
	case "colored":
		program = ColoredListProgram{}
	case "nerdfont":
		program = NerdfontListProgram{}
	default:
		program = NerdfontListProgram{}
	}

	orc.ProcessDiscoveredManifests()

	filter := CreateListFilter(kind)
	defer filter.Close()

	rg := orc.GetResourceGraph()
	resources := rg.ListResourcesWithOptionalSelector(filter)
	if _, err := program.Run(resources); err != nil {
		fmt.Printf("Error running program: %v\n", err)
		os.Exit(1)
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

func CreateListCommand(kinds []string) *cobra.Command {
	listCmd := &cobra.Command{
		Use:   "list",
		Short: "List resources in the graph",
		Aliases: []string{
			"ls",
		},
		GroupID: "inspection",
	}
	listCmd.PersistentFlags().StringP("format", "f", "nerdfont", "Print using colors and nerdfonts")

	for _, kind := range kinds {
		listCmd.AddCommand(CreateListResourcesByKindCommand(kind))
	}

	return listCmd
}
