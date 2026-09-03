package cli

import (
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/lrstanley/go-nf/glyphs/fa"
)

type ValidateStyle struct {
	Row       lg.Style
	Valid     lg.Style
	Invalid   lg.Style
	Filename  lg.Style
	Message   lg.Style
	Error     lg.Style
	Separator lg.Style
}

func NewValidateStyle() *ValidateStyle {
	rowStyle := lg.NewStyle().
		MarginLeft(2)
	indStyle := lg.NewStyle().
		Bold(true)
	return &ValidateStyle{
		Row: rowStyle,
		Valid: indStyle.
			Foreground(lg.Color("#d4edda")),
		Invalid: indStyle.
			Foreground(lg.Color("#f8d7da")),
		Filename: lg.NewStyle().
			MarginLeft(1),
		Message: lg.NewStyle(),
		Separator: lg.NewStyle().
			MarginLeft(1).
			MarginRight(1),
		Error: rowStyle.
			MarginLeft(rowStyle.GetMarginLeft() + 2),
	}
}

func (style *ValidateStyle) PrintError(err error) {
	errStyle := lg.NewStyle().
		Foreground(lg.Color("#f8d7da"))
	errMessageStyle := lg.NewStyle().
		MarginLeft(1).
		Bold(true).
		Foreground(lg.Color("#FFFFFF"))
	fmt.Println(style.Row.Render(lg.JoinHorizontal(
		lg.Left,
		errStyle.Render(fa.TimesCircle.String()),
		errMessageStyle.Render(err.Error()),
	)))
}

func (style *ValidateStyle) GetResultStyle(result hypha.SchemaValidationResult) *lg.Style {
	if result.IsValid {
		return &style.Valid
	}

	return &style.Invalid
}

func (style *ValidateStyle) GetResultGlyph(result hypha.SchemaValidationResult) string {
	if result.IsValid {
		return fa.Check.String()
	}

	return fa.Times.String()
}

func (style *ValidateStyle) PrintResult(result hypha.SchemaValidationResult) {
	ind := style.GetResultGlyph(result)
	indStyle := style.GetResultStyle(result)
	if result.IsValid {
		fmt.Println(style.Row.Render(indStyle.Render(lg.JoinHorizontal(
			lg.Left,
			ind,
			style.Filename.Render(result.Filename),
		))))
	} else {
		fmt.Println(style.Row.Render(lg.JoinHorizontal(
			lg.Left,
			indStyle.Render(lg.JoinHorizontal(
				lg.Left,
				ind,
				style.Filename.Render(result.Filename),
			)),
			style.Separator.Render("---"),
			style.Message.Render(result.Error.Error()),
		)))
	}
}

func (style *ValidateStyle) PrintSummary(summary *ValidateSummary) {
	validInd := fa.Check.String()
	invalidInd := fa.Times.String()

	if summary.IsInvalid() {
		fmt.Println(style.Row.Render(style.Invalid.Render(lg.JoinHorizontal(
			lg.Left,
			invalidInd,
			style.Message.Render("All manifests are invalid"),
		))))
	} else if summary.IsValid() {
		fmt.Println(style.Row.Render(style.Valid.Render(lg.JoinHorizontal(
			lg.Left,
			validInd,
			style.Message.Render("All manifests are valid"),
		))))
	} else {
		fmt.Println(style.Row.Render(style.Valid.Render(lg.JoinHorizontal(
			lg.Left,
			validInd,
			style.Message.Render(fmt.Sprintf("%d/%d manifests are valid", summary.Valid, summary.Total)),
		))))
		fmt.Println(style.Row.Render(style.Invalid.Render(lg.JoinHorizontal(
			lg.Left,
			invalidInd,
			style.Message.Render(fmt.Sprintf("%d/%d manifests are invalid", summary.Invalid, summary.Total)),
		))))
	}
}

func (style *ValidateStyle) PrintResults(results []hypha.SchemaValidationResult) *ValidateSummary {
	summary := &ValidateSummary{}
	for _, result := range results {
		style.PrintResult(result)
		if result.IsValid {
			summary.Valid++
		} else {
			summary.Invalid++
		}

		summary.Total++
		fmt.Println()
	}

	return summary
}
