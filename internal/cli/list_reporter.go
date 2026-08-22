package cli

import (
	"encoding/json"
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/viper"
	"gopkg.in/yaml.v3"
)

func PrintResourceList(style *ListStyle, resources []hypha.Resource) {
	fmt.Println()
	style.Print(resources)
	fmt.Println()
}

type ListReporter interface {
	Run(resources []hypha.Resource) (any, error)
}

type PlainListReporter struct{}

func (p PlainListReporter) Run(resources []hypha.Resource) (any, error) {
	cs := hypha.GetDefaultColorScheme()
	rowStyle := lg.NewStyle().
		MarginLeft(2)
	style := NewListStyle(&rowStyle, &cs)
	PrintResourceList(style, resources)
	return nil, nil
}

type PrettyListReporter struct{}

func (p PrettyListReporter) Run(resources []hypha.Resource) (any, error) {
	cs := hypha.GetDefaultColorScheme()
	rowStyle := lg.NewStyle().
		MarginLeft(2)
	style := NewListStyle(&rowStyle, &cs)
	PrintResourceList(style, resources)
	return nil, nil
}

type ColoredListReporter struct{}

func (p ColoredListReporter) Run(resources []hypha.Resource) (any, error) {
	cs := hypha.GetDefaultColorScheme()
	rowStyle := lg.NewStyle().
		MarginLeft(2)
	style := NewListStyle(&rowStyle, &cs)
	PrintResourceList(style, resources)
	return nil, nil
}

type JsonListReporter struct{}

func (p JsonListReporter) Run(resources []hypha.Resource) (any, error) {
	bytes, err := json.MarshalIndent(resources, "", "  ")
	if err != nil {
		return nil, err
	}

	fmt.Println(string(bytes))
	return nil, nil
}

type JsonLinesListReporter struct{}

func (p JsonLinesListReporter) Run(resources []hypha.Resource) (any, error) {
	for _, res := range resources {
		bytes, err := json.Marshal(res)
		if err != nil {
			return nil, err
		}

		fmt.Println(string(bytes))
	}

	return nil, nil
}

type YamlListReporter struct{}

func (p YamlListReporter) Run(resources []hypha.Resource) (any, error) {
	bytes, err := yaml.Marshal(resources)
	if err != nil {
		return nil, err
	}

	fmt.Println(string(bytes))
	return nil, nil
}

func GetListReporter() ListReporter {
	format := viper.GetString("format")
	switch format {
	case "json":
		return JsonListReporter{}
	case "jsonl":
		return JsonLinesListReporter{}
	case "yaml":
		return YamlListReporter{}
	case "plain":
		return PlainListReporter{}
	case "colored":
		return ColoredListReporter{}
	case "pretty":
		return PrettyListReporter{}
	default:
		return PrettyListReporter{}
	}
}
