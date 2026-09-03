package cli

import (
	"fmt"
	"os"
	"text/template"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/lrstanley/go-nf/glyphs/fa"
)

type GenerationStatus int

const (
	GenerationSkipped GenerationStatus = iota
	GenerationSuccess
	GenerationFailed
)

func (status GenerationStatus) String() string {
	switch status {
	case GenerationSkipped:
		return "Skipped"
	case GenerationSuccess:
		return "Success"
	case GenerationFailed:
		return "Failed"
	default:
		return "Unknown"
	}
}

func (status GenerationStatus) Glyph() string {
	switch status {
	case GenerationSkipped:
		return fa.FastForward.String()
	case GenerationSuccess:
		return fa.Check.String()
	case GenerationFailed:
		return fa.TimesCircle.String()
	default:
		return fa.QuestionCircle.String()
	}
}

type GenerationResult struct {
	Filename string           `json:"filename" yaml:"filename"`
	Status   GenerationStatus `json:"status" yaml:"status"`
	Error    error            `json:"error" yaml:"error"`
}

func WriteTemplate(filename string, tpl string, ctx any) error {
	flags := os.O_RDWR | os.O_CREATE
	file, err := os.OpenFile(filename, flags, 0644)
	if err != nil {
		return err
	}

	defer file.Close()
	template := template.Must(template.New(fmt.Sprintf("%s.tpl", filename)).Parse(tpl))
	return template.Execute(file, ctx)
}

func Generate(filename string, template string, ctx any) GenerationResult {
	if !hypha.FileExists(filename) {
		if err := WriteTemplate(filename, template, ctx); err != nil {
			return GenerationResult{
				Filename: filename,
				Status:   GenerationFailed,
				Error:    err,
			}
		}

		return GenerationResult{
			Filename: filename,
			Status:   GenerationSuccess,
			Error:    nil,
		}
	}

	return GenerationResult{
		Filename: filename,
		Status:   GenerationSkipped,
		Error:    nil,
	}
}
