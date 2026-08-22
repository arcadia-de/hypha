package cli

import (
	lg "charm.land/lipgloss/v2"
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
)

type ListStyle struct {
	RowStyle         lg.Style
	HeaderRowStyle   lg.Style
	HeaderStateStyle lg.Style
	HeaderKindStyle  lg.Style
	HeaderNameStyle  lg.Style
	HeaderIdStyle    lg.Style
	PendingStyle     lg.Style
	ProcessingStyle  lg.Style
	ReadyStyle       lg.Style
	FailedStyle      lg.Style
	UnknownStyle     lg.Style
	KindStyle        lg.Style
	NameStyle        lg.Style
	IdStyle          lg.Style
}

func NewListStyle(rowStyle *lg.Style, cs *hypha.ColorScheme) *ListStyle {
	const (
		KindWidth  = 20
		StateWidth = 16
		NameWidth  = 20
		IdWidth    = 40
	)

	colStyle := lg.NewStyle().
		Padding(0, 1)

	stateStyle := colStyle.
		Width(StateWidth).
		Align(lg.Right)

	headerStyle := colStyle
	return &ListStyle{
		RowStyle: (*rowStyle),
		HeaderRowStyle: (*rowStyle).
			BorderStyle(lg.NormalBorder()).
			BorderBottom(true).
			BorderForeground(cs.GetBorderColor()),

		HeaderStateStyle: headerStyle.
			Width(StateWidth).
			Align(lg.Right),
		HeaderKindStyle: headerStyle.
			Width(KindWidth).
			Align(lg.Center),
		HeaderNameStyle: headerStyle.
			Width(NameWidth).
			Align(lg.Left),
		HeaderIdStyle: headerStyle.
			Width(IdWidth).
			Align(lg.Left),

		PendingStyle: stateStyle.
			Foreground(cs.GetPendingColor()),
		ProcessingStyle: stateStyle.
			Foreground(cs.GetProcessingColor()),
		ReadyStyle: stateStyle.
			Foreground(cs.GetReadyColor()),
		FailedStyle: stateStyle.
			Foreground(cs.GetFailedColor()),
		UnknownStyle: stateStyle.
			Foreground(cs.GetUnknownColor()),

		KindStyle: colStyle.
			Align(lg.Center).
			Width(KindWidth),
		NameStyle: colStyle.
			Align(lg.Left).
			Width(NameWidth),
		IdStyle: colStyle.
			MaxWidth(IdWidth).
			Align(lg.Left),
	}
}

func (style *ListStyle) GetStateStyle(state string) *lg.Style {
	switch state {
	case "Pending":
		return &style.PendingStyle
	case "Processing":
		return &style.ProcessingStyle
	case "Ready":
		return &style.ReadyStyle
	case "Failed":
		return &style.FailedStyle
	case "Unknown":
		return &style.UnknownStyle
	default:
		return &style.UnknownStyle
	}
}

func (style *ListStyle) PrintRow(value string) {
	fmt.Println(style.RowStyle.Render(value))
}

func (style *ListStyle) Print(resources []hypha.Resource) {
	fmt.Println(style.HeaderRowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		style.HeaderStateStyle.Render("State"),
		style.HeaderKindStyle.Render("Kind"),
		style.HeaderNameStyle.Render("Name"),
		style.HeaderIdStyle.Render("ID"),
	)))

	for _, resource := range resources {
		stateStyle := style.GetStateStyle(resource.State)
		style.PrintRow(lg.JoinHorizontal(
			lg.Left,
			stateStyle.Render(resource.State),
			style.KindStyle.Render(resource.Kind),
			style.NameStyle.Render(resource.Metadata.Name),
			style.IdStyle.Render(resource.ID),
		))
	}
}
