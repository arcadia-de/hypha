package cli

import (
	"encoding/json"
	"fmt"
	"os"

	lg "charm.land/lipgloss/v2"
	"github.com/lrstanley/go-nf/glyphs/md"
	"github.com/spf13/viper"
)

func HandleQueryFailed(query string, err error) {
	rowStyle := lg.NewStyle().
		MarginLeft(4)

	subrowStyle := lg.NewStyle().
		MarginLeft(6)

	prefixStyle := lg.NewStyle().
		Bold(true)

	fmt.Println()
	fmt.Println(rowStyle.Render("Query Failed"))
	fmt.Println()
	fmt.Println(rowStyle.Render(prefixStyle.Render("expression:")))
	fmt.Println(subrowStyle.Render(query))
	fmt.Println()
	fmt.Println(rowStyle.Render(prefixStyle.Render("reason:")))
	fmt.Println(subrowStyle.Render(err.Error()))
	fmt.Println()
}

type QueryResultReporter interface {
	Handle(query string, results any)
}

func PrintQueryResultString(status string, query string, results any, rowStyle *lg.Style, statusStyle *lg.Style) {
	var data any
	switch results.(type) {
	case string:
		if err := json.Unmarshal([]byte(results.(string)), &data); err != nil {
			os.Exit(1)
		}
	default:
		data = results
	}

	content, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		HandleQueryFailed(query, fmt.Errorf("failed to marshal results %v: %v", results, err))
		return
	}

	subrowStyle := rowStyle.MarginLeft(rowStyle.GetMarginLeft() + 2)
	topicStyle := rowStyle.Bold(true)

	fmt.Println()
	fmt.Println(rowStyle.Render(statusStyle.Render(status)))
	fmt.Println()

	fmt.Println(topicStyle.Render("Query:"))
	fmt.Println(subrowStyle.Render(query))
	fmt.Println()

	fmt.Println(topicStyle.Render("Result:"))
	fmt.Println(subrowStyle.Render(string(content)))
	fmt.Println()
}

type PrettyQueryResultReporter struct{}

func (reporter *PrettyQueryResultReporter) Handle(query string, results any) {
	var data any
	switch results.(type) {
	case string:
		if err := json.Unmarshal([]byte(results.(string)), &data); err != nil {
			os.Exit(1)
		}
	default:
		data = results
	}

	content, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		HandleQueryFailed(query, fmt.Errorf("failed to marshal results %v: %v", results, err))
		return
	}

	rowStyle := lg.NewStyle().
		MarginLeft(4)
	subrowStyle := lg.NewStyle().
		MarginLeft(6)

	resultStyle := lg.NewStyle().
		Foreground(lg.Color("#00FF00")).
		Bold(true)

	indStyle := resultStyle.
		MarginRight(1)

	valueStyle := subrowStyle

	fmt.Println()
	fmt.Println(rowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		indStyle.Render(md.CheckCircle.String()),
		resultStyle.Render("Success:"),
	)))
	fmt.Println()
	fmt.Println(valueStyle.Render(string(content)))
	fmt.Println()
}

type PlainQueryResultReporter struct{}

func (reporter *PlainQueryResultReporter) Handle(query string, results any) {
	var data any
	switch results.(type) {
	case string:
		if err := json.Unmarshal([]byte(results.(string)), &data); err != nil {
			os.Exit(1)
		}
	default:
		data = results
	}

	content, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		HandleQueryFailed(query, fmt.Errorf("failed to marshal results %v: %v", results, err))
		return
	}

	rowStyle := lg.NewStyle().
		MarginLeft(4)

	fmt.Println()
	fmt.Println(rowStyle.Render(string(content)))
	fmt.Println()
}

type ColoredQueryResultReporter struct{}

func (reporter *ColoredQueryResultReporter) Handle(query string, results any) {

}

type JsonQueryResultReporter struct {
}

func (reporter *JsonQueryResultReporter) Handle(query string, results any) {
	var data any
	switch results.(type) {
	case string:
		if err := json.Unmarshal([]byte(results.(string)), &data); err != nil {
			os.Exit(1)
		}
	default:
		data = results
	}

	content, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		HandleQueryFailed(query, fmt.Errorf("failed to marshal results %v: %v", results, err))
		return
	}

	fmt.Println(string(content))
}

type JsonlQueryResultReporter struct {
}

func (reporter *JsonlQueryResultReporter) Handle(query string, results any) {
	var data any
	switch results.(type) {
	case string:
		if err := json.Unmarshal([]byte(results.(string)), &data); err != nil {
			os.Exit(1)
		}
	default:
		data = results
	}

	content, err := json.Marshal(data)
	if err != nil {
		HandleQueryFailed(query, fmt.Errorf("failed to marshal results %v: %v", results, err))
		return
	}

	fmt.Println(string(content))
}

func NewQueryResultReporter() QueryResultReporter {
	format := viper.GetString("format")
	switch format {
	case "json":
		return &JsonQueryResultReporter{}
	case "jsonl":
		return &JsonlQueryResultReporter{}
	case "plain":
		return &PlainQueryResultReporter{}
	case "colored":
		return &ColoredQueryResultReporter{}
	case "pretty":
		return &PrettyQueryResultReporter{}
	default:
		return &PrettyQueryResultReporter{}
	}
}
