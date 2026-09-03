package cli

import (
	"fmt"

	lg "charm.land/lipgloss/v2"
)

type InitStyle struct {
	Format        OutputFormat
	Row           lg.Style
	ErrorInd      lg.Style
	ErrorMessage  lg.Style
	Filename      lg.Style
	SkippedStatus lg.Style
	SuccessStatus lg.Style
	FailedStatus  lg.Style
	UnknownStatus lg.Style
}

func (style *InitStyle) IsPlain() bool {
	return style.Format == OutFormatPlain
}

func (style *InitStyle) IsColored() bool {
	return style.Format == OutFormatColored
}

func (style *InitStyle) IsPretty() bool {
	return style.Format == OutFormatPretty
}

func (style *InitStyle) IsJson() bool {
	return style.Format == OutFormatJson
}

func (style *InitStyle) IsJsonl() bool {
	return style.Format == OutFormatJsonl
}

func (style *InitStyle) IsYaml() bool {
	return style.Format == OutFormatYaml
}

func (style *InitStyle) GetStatusStyle(status GenerationStatus) *lg.Style {
	switch status {
	case GenerationSkipped:
		return &style.SkippedStatus
	case GenerationSuccess:
		return &style.SuccessStatus
	case GenerationFailed:
		return &style.FailedStatus
	default:
		return &style.UnknownStatus
	}
}

func (style *InitStyle) PrintGenerationResult(result GenerationResult) {
	switch result.Status {
	case GenerationFailed:
		fmt.Println(style.Row.Render(lg.JoinHorizontal(
			lg.Left,
			style.GetStatusStyle(result.Status).Render(result.Status.Glyph()),
			style.GetStatusStyle(result.Status).Render(result.Status.String()),
			style.Filename.Render(result.Filename),
			":",
			result.Error.Error(),
		)))
	default:
		filenameStyle := style.Filename
		if result.Status == GenerationSkipped {
			filenameStyle = filenameStyle.
				Bold(false).
				Foreground(lg.Color("#C3C3C3"))
		}

		fmt.Println(style.Row.Render(lg.JoinHorizontal(
			lg.Left,
			style.GetStatusStyle(result.Status).Render(result.Status.Glyph()),
			style.GetStatusStyle(result.Status).Render(result.Status.String()),
			filenameStyle.Render(result.Filename),
		)))
	}
}

func NewInitStyle() *InitStyle {
	rowStyle := lg.NewStyle().
		MarginLeft(2)
	filenameStyle := lg.NewStyle().
		MarginLeft(1).
		Bold(true)
	errIndStyle := lg.NewStyle().
		MarginLeft(1).
		Bold(true).
		Foreground(lg.Color("#FF0000"))
	errMessageStyle := lg.NewStyle().
		MarginLeft(1)

	baseStatusStyle := lg.NewStyle().
		MarginLeft(1).
		Bold(true)
	skippedStyle := baseStatusStyle.
		Foreground(lg.Color("#0000FF"))
	successStyle := baseStatusStyle.
		Foreground(lg.Color("#00FF00"))
	failedStyle := baseStatusStyle.
		Foreground(lg.Color("#FF0000"))

	return &InitStyle{
		Row:           rowStyle,
		ErrorInd:      errIndStyle,
		ErrorMessage:  errMessageStyle,
		Filename:      filenameStyle,
		SkippedStatus: skippedStyle,
		SuccessStatus: successStyle,
		FailedStatus:  failedStyle,
		UnknownStatus: baseStatusStyle,
	}
}
