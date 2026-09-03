package cli

import (
	"encoding/json"
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/lrstanley/go-nf/glyphs/fa"
	"github.com/spf13/cobra"
	"gopkg.in/yaml.v3"

	"github.com/arcadia-de/hypha/internal/hypha"
)

type GenerateRequest struct {
	Filename string
	Template string
	Context  any
}

func HandleInit(cmd *cobra.Command, args []string) error {
	format := GetOutFormat()
	style := NewInitStyle()

	config_dir, err := hypha.EnsureConfigDirExists()
	if err != nil {
		fmt.Println()
		fmt.Println(style.Row.Render(lg.JoinHorizontal(
			lg.Left,
			style.ErrorInd.Render(fa.TimesCircle.String()),
			style.ErrorMessage.Render(fmt.Sprintf("failed to initialize hypha config: %v", err)),
		)))
		fmt.Println()
		return nil
	}

	requests := []GenerateRequest{
		GenerateRequest{
			Filename: fmt.Sprintf("%s/init.lua", config_dir),
			Template: `
      print('Hello World')
      `,
			Context: nil,
		},
	}
	results := []GenerationResult{}
	for _, req := range requests {
		results = append(results, Generate(req.Filename, req.Template, req.Context))
	}

	if format == OutFormatJson {
		bytes, err := json.MarshalIndent(results, "", "  ")
		if err != nil {
			fmt.Println()
			fmt.Println(style.Row.Render(lg.JoinHorizontal(
				lg.Left,
				style.ErrorInd.Render(fa.TimesCircle.String()),
				style.ErrorMessage.Render(fmt.Sprintf("failed to marshal json: %v", err)),
			)))
			fmt.Println()
			return nil
		}

		fmt.Println(string(bytes))
	} else if format == OutFormatYaml {
		bytes, err := yaml.Marshal(results)
		if err != nil {
			fmt.Println()
			fmt.Println(style.Row.Render(lg.JoinHorizontal(
				lg.Left,
				style.ErrorInd.Render(fa.TimesCircle.String()),
				style.ErrorMessage.Render(fmt.Sprintf("failed to marshal yaml: %v", err)),
			)))
			fmt.Println()
			return nil
		}

		fmt.Println(string(bytes))
	} else if format == OutFormatJsonl {
		lines := []string{}
		for _, res := range results {
			bytes, err := json.Marshal(res)
			if err != nil {
				fmt.Println()
				fmt.Println(style.Row.Render(lg.JoinHorizontal(
					lg.Left,
					style.ErrorInd.Render(fa.TimesCircle.String()),
					style.ErrorMessage.Render(fmt.Sprintf("failed to marshal json: %v", err)),
				)))
				fmt.Println()
				return nil
			}

			lines = append(lines, string(bytes))
		}

		for _, line := range lines {
			fmt.Println(line)
		}
	} else {
		fmt.Println()
		for _, res := range results {
			style.PrintGenerationResult(res)
		}
		fmt.Println()
	}

	return nil
}

func init() {
	initCmd := &cobra.Command{
		Use:     "init",
		Short:   "Initialize hypha on a system",
		GroupID: "config",
		RunE:    HandleInit,
	}
	initCmd.Flags().StringP("format", "f", DefaultOutputFormatString, "The output format. Values are: plain, colored, pretty, json, jsonl, yaml (default: pretty)")
	initCmd.Flags().BoolP("plain", "", false, "Set the output format to plain")
	initCmd.Flags().BoolP("colored", "", false, "Set the output format to colored")
	initCmd.Flags().BoolP("pretty", "", false, "Set the output format to pretty")
	initCmd.Flags().BoolP("json", "", false, "Set the output format to colored")
	initCmd.Flags().BoolP("jsonl", "", false, "Set the output format to jsonl")
	initCmd.Flags().BoolP("yaml", "", false, "Set the output format to yaml")
	initCmd.MarkFlagsMutuallyExclusive("format", "plain", "colored", "pretty", "json", "jsonl", "yaml")

	RootCmd.AddCommand(initCmd)
}
