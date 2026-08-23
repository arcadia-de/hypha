package cli

import (
	"fmt"
	"strings"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	md "github.com/lrstanley/go-nf/glyphs/md"
)

type HistoryStyle struct {
	RowStyle             lg.Style
	SelectedRowStyle     lg.Style
	HeaderRowStyle       lg.Style
	HeaderActionStyle    lg.Style
	HeaderStatusStyle    lg.Style
	HeaderKindStyle      lg.Style
	HeaderIdStyle        lg.Style
	HeaderAppliedAtStyle lg.Style
	StatusStyle          lg.Style
	NoActionStyle        lg.Style
	CreateActionStyle    lg.Style
	UpdateActionStyle    lg.Style
	DestroyActionStyle   lg.Style
	KindStyle            lg.Style
	IdStyle              lg.Style
}

func NewHistoryStyle(rowStyle *lg.Style, cs *hypha.ColorScheme) *HistoryStyle {
	const (
		StatusWidth = 15
		ActionWidth = 15
		KindWidth   = 15
		IdWidth     = 50
	)

	colStyle := lg.NewStyle().
		Padding(0, 1)
	statusStyle := colStyle.
		Align(lg.Center).
		Width(StatusWidth)
	actionStyle := colStyle.
		Align(lg.Center).
		Width(ActionWidth)
	return &HistoryStyle{
		RowStyle: (*rowStyle),
		SelectedRowStyle: (*rowStyle).
			Background(lg.Color("#333333")).
			Bold(true),
		HeaderRowStyle: (*rowStyle).
			BorderBottom(true).
			BorderStyle(lg.NormalBorder()).
			BorderForeground(cs.GetBorderColor()),

		HeaderStatusStyle: colStyle.
			Align(lg.Center).
			Width(StatusWidth),
		HeaderActionStyle: colStyle.
			Align(lg.Center).
			Width(ActionWidth),
		HeaderKindStyle: colStyle.
			Align(lg.Center).
			Width(KindWidth),
		HeaderIdStyle: colStyle.
			Align(lg.Left).
			Width(IdWidth),

		StatusStyle: statusStyle,
		NoActionStyle: actionStyle.
			Foreground(cs.GetNoneColor()),
		CreateActionStyle: actionStyle.
			Foreground(cs.GetCreateColor()),
		UpdateActionStyle: actionStyle.
			Foreground(cs.GetUpdateColor()),
		DestroyActionStyle: actionStyle.
			Foreground(cs.GetDestroyColor()),
		KindStyle: colStyle.
			Align(lg.Center).
			Width(KindWidth),
		IdStyle: colStyle.
			Align(lg.Left).
			Width(IdWidth),
	}
}

func (style *HistoryStyle) GetActionIcon(action string) string {
	switch action {
	case "None":
		return md.Minus.String()
	case "Create":
		return md.Plus.String()
	case "Update":
		return md.Pencil.String()
	case "Destroy":
		return md.Delete.String()
	default:
		return ""
	}
}

func (style *HistoryStyle) GetActionStyle(status string) *lg.Style {
	switch status {
	case "None":
		return &style.NoActionStyle
	case "Create":
		return &style.CreateActionStyle
	case "Update":
		return &style.UpdateActionStyle
	case "Destroy":
		return &style.DestroyActionStyle
	default:
		return &style.NoActionStyle
	}
}

func (style *HistoryStyle) PrintTo(records []hypha.HistoryRecord, selected int, num int, sb *strings.Builder) {
	fmt.Fprintln(sb)
	fmt.Fprintf(sb, "%s\n", style.HeaderRowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		style.HeaderStatusStyle.Render("Status"),
		style.HeaderActionStyle.Render("Action"),
		style.HeaderKindStyle.Render("Kind"),
		style.HeaderIdStyle.Render("ID"),
	)))

	for i, record := range records[0:min(len(records), num)] {
		actionStyle := style.GetActionStyle(record.Action)
		row := lg.JoinHorizontal(
			lg.Left,
			style.StatusStyle.Render(fmt.Sprintf("%s %s", record.Status.GetIcon(), record.Status.GetName())),
			actionStyle.Render(fmt.Sprintf("%s %s", style.GetActionIcon(record.Action), record.Action)),
			style.KindStyle.Render(record.Kind),
			style.IdStyle.Render(record.ID),
		)

		var rowStyle lg.Style
		if i == selected {
			rowStyle = style.SelectedRowStyle
		} else {
			rowStyle = style.RowStyle
		}
		fmt.Fprintf(sb, "%s\n", rowStyle.Render(row))
	}
}
