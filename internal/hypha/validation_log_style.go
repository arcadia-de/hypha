package hypha

import (
	lg "charm.land/lipgloss/v2"
	"fmt"
)

type ValidationLogStyle struct {
	RowStyle          lg.Style
	HeaderRowStyle    lg.Style
	HeaderKindStyle   lg.Style
	HeaderNameStyle   lg.Style
	HeaderReasonStyle lg.Style
	SkippedKindStyle  lg.Style
	PassedKindStyle   lg.Style
	FailedKindStyle   lg.Style
	WarningKindStyle  lg.Style
	UnknownKindStyle  lg.Style
	NameStyle         lg.Style
	ReasonStyle       lg.Style
}

func NewValidationLogStyle(rowStyle *lg.Style, cs *ColorScheme) *ValidationLogStyle {
	colStyle := lg.NewStyle().
		Padding(0, 1)
	headerStyle := colStyle
	kindStyle := colStyle.
		Align(lg.Right).
		Width(14)
	return &ValidationLogStyle{
		RowStyle: *rowStyle,
		HeaderRowStyle: (*rowStyle).
			BorderBottom(true).
			BorderStyle(lg.NormalBorder()).
			BorderForeground(cs.GetBorderColor()),
		HeaderKindStyle: headerStyle.
			Width(14).
			Align(lg.Right),
		HeaderNameStyle: headerStyle.
			Width(20).
			Align(lg.Center),
		HeaderReasonStyle: headerStyle.
			Width(50).
			Align(lg.Left),

		SkippedKindStyle: kindStyle.
			Foreground(cs.GetSkippedColor()),
		PassedKindStyle: kindStyle.
			Foreground(cs.GetPassedColor()),
		FailedKindStyle: kindStyle.
			Foreground(cs.GetFailedColor()),
		WarningKindStyle: kindStyle.
			Foreground(cs.GetWarningColor()),
		UnknownKindStyle: kindStyle.
			Foreground(cs.GetUnknownColor()),
		NameStyle: colStyle.
			Align(lg.Center).
			Width(20),
		ReasonStyle: colStyle.
			Align(lg.Left).
			MaxWidth(50).
			Foreground(cs.GetMutedColor()),
	}
}

func (style *ValidationLogStyle) PrintRow(result *ValidationResult, verbose bool) {
	kind := result.Kind
	if kind == PassedValidationResult {
		if !verbose {
			return
		}
	}

	var kindStyle lg.Style
	switch kind {
	case SkippedValidationResult:
		kindStyle = style.SkippedKindStyle
	case PassedValidationResult:
		kindStyle = style.PassedKindStyle
	case FailedValidationResult:
		kindStyle = style.FailedKindStyle
	case WarningValidationResult:
		kindStyle = style.WarningKindStyle
	default:
		kindStyle = style.UnknownKindStyle
	}

	fmt.Println(style.RowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		kindStyle.Render(fmt.Sprintf("%s %s", GetValidationResultKindIcon(kind), GetValidationResultKindName(kind))),
		style.NameStyle.Render(result.Name),
		style.ReasonStyle.Render(result.Reason),
	)))
}
