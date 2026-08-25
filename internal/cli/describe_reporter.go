package cli

import (
	"encoding/json"
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/viper"
	"gopkg.in/yaml.v3"
)

type DescribeReporter interface {
	Run(resources []hypha.Resource) error
}

type PlainDescribeReporter struct{}

func (reporter PlainDescribeReporter) Run(resources []hypha.Resource) error {
	fmt.Println()

	rowStyle := lg.NewStyle().
		MarginLeft(2)

	style := NewDescribeStyle(&rowStyle)

	if len(resources) == 0 {
		fmt.Println("No resources found")
		return nil
	}

	for _, res := range resources {
		style.Print(res)
		fmt.Println()
	}

	return nil
}

type ColoredDescribeReporter struct{}

func (reporter ColoredDescribeReporter) Run(resources []hypha.Resource) error {
	fmt.Println()

	rowStyle := lg.NewStyle().
		MarginLeft(2)

	style := NewDescribeStyle(&rowStyle)

	if len(resources) == 0 {
		fmt.Println("No resources found")
		return nil
	}

	for _, res := range resources {
		style.Print(res)
		fmt.Println()
	}

	return nil
}

type PrettyDescribeReporter struct{}

func (reporter PrettyDescribeReporter) Run(resources []hypha.Resource) error {
	fmt.Println()

	rowStyle := lg.NewStyle().
		MarginLeft(2)

	style := NewDescribeStyle(&rowStyle)

	if len(resources) == 0 {
		fmt.Println("No resources found")
		return nil
	}

	for _, res := range resources {
		style.Print(res)
		fmt.Println()
	}

	return nil
}

type JsonDescribeReporter struct{}

func (reporter JsonDescribeReporter) Run(resources []hypha.Resource) error {
	bytes, err := json.MarshalIndent(resources, "", "  ")
	if err != nil {
		return err
	}

	fmt.Println(string(bytes))
	return nil
}

type JsonLinesDescribeReporter struct{}

func (reporter JsonLinesDescribeReporter) Run(resources []hypha.Resource) error {
	for _, res := range resources {
		bytes, err := json.Marshal(res)
		if err != nil {
			return err
		}

		fmt.Println(string(bytes))
	}

	return nil
}

type YamlDescribeReporter struct{}

func (reporter YamlDescribeReporter) Run(resources []hypha.Resource) error {
	bytes, err := yaml.Marshal(resources)
	if err != nil {
		return err
	}

	fmt.Println(string(bytes))
	return nil
}

func GetDescribeReporter() DescribeReporter {
	format := viper.GetString("format")
	switch format {
	case "json":
		return JsonDescribeReporter{}
	case "jsonl":
		return JsonLinesDescribeReporter{}
	case "yaml":
		return YamlDescribeReporter{}
	case "plain":
		return PlainDescribeReporter{}
	case "colored":
		return ColoredDescribeReporter{}
	case "pretty":
		return PrettyDescribeReporter{}
	default:
		return PrettyDescribeReporter{}
	}
}
