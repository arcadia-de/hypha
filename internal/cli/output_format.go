package cli

import (
	"strings"

	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

type OutputFormat int

const (
	OutFormatPlain OutputFormat = iota
	OutFormatColored
	OutFormatPretty
	OutFormatJson
	OutFormatJsonl
	OutFormatYaml
)

const (
	DefaultOutputFormatString = "pretty"
)

func ParseOutputFormat(format string) OutputFormat {
	switch strings.ToLower(format) {
	case "plain":
		return OutFormatPlain
	case "colored":
		return OutFormatColored
	case "pretty":
		return OutFormatPretty
	case "json":
		return OutFormatJson
	case "jsonl":
		return OutFormatJsonl
	case "yaml":
		return OutFormatYaml
	default:
		return OutFormatPlain
	}
}

func GetOutFormat() OutputFormat {
	if set := viper.GetBool("plain"); set {
		return OutFormatPlain
	} else if set := viper.GetBool("colored"); set {
		return OutFormatColored
	} else if set := viper.GetBool("pretty"); set {
		return OutFormatPretty
	} else if set := viper.GetBool("json"); set {
		return OutFormatJson
	} else if set := viper.GetBool("jsonl"); set {
		return OutFormatJsonl
	} else if set := viper.GetBool("yaml"); set {
		return OutFormatYaml
	}

	return ParseOutputFormat(viper.GetString("format"))
}

func AddFormatFlags(c *cobra.Command) {
	c.Flags().StringP("format", "f", DefaultOutputFormatString, "The output format. Values are: plain, colored, pretty, json, jsonl, yaml (default: pretty)")
	c.Flags().BoolP("plain", "", false, "Set the output format to plain")
	c.Flags().BoolP("colored", "", false, "Set the output format to colored")
	c.Flags().BoolP("pretty", "", false, "Set the output format to pretty")
	c.Flags().BoolP("json", "", false, "Set the output format to colored")
	c.Flags().BoolP("jsonl", "", false, "Set the output format to jsonl")
	c.Flags().BoolP("yaml", "", false, "Set the output format to yaml")
	c.MarkFlagsMutuallyExclusive("format", "plain", "colored", "pretty", "json", "jsonl", "yaml")
}
