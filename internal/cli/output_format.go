package cli

import (
	"strings"

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
