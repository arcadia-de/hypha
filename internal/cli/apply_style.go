package cli

import (
	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
)

type ApplyStyle struct {
	RowStyle           lg.Style
	HeaderRowStyle     lg.Style
	HeaderActionStyle  lg.Style
	HeaderIdStyle      lg.Style
	HeaderKindStyle    lg.Style
	HeaderNameStyle    lg.Style
	HeaderReasonStyle  lg.Style
	NoActionStyle      lg.Style
	CreateActionStyle  lg.Style
	UpdateActionStyle  lg.Style
	DestroyActionStyle lg.Style
	FailedActionStyle  lg.Style
	KindStyle          lg.Style
	NameStyle          lg.Style
	IdStyle            lg.Style
	ReasonStyle        lg.Style
}

func NewApplyStyle(rowStyle *lg.Style, cs *hypha.ColorScheme) *ApplyStyle {
	colStyle := lg.NewStyle().
		Padding(0, 1)
	actionStyle := colStyle.
		Width(14).
		Align(lg.Right)
	return &ApplyStyle{
		RowStyle: (*rowStyle),
		HeaderRowStyle: (*rowStyle).
			BorderBottom(true).
			BorderStyle(lg.NormalBorder()).
			BorderForeground(cs.GetBorderColor()),
		HeaderActionStyle: colStyle.
			Width(14).
			Align(lg.Right),
		HeaderNameStyle: colStyle.
			Width(20).
			Align(lg.Center),
		HeaderIdStyle: colStyle.
			Width(20).
			Align(lg.Center),
		HeaderKindStyle: colStyle.
			Width(20).
			Align(lg.Center),
		HeaderReasonStyle: colStyle.
			Width(50).
			Align(lg.Left),

		NoActionStyle: actionStyle.
			Foreground(cs.GetNoneColor()),
		CreateActionStyle: actionStyle.
			Foreground(cs.GetCreateColor()),
		UpdateActionStyle: actionStyle.
			Foreground(cs.GetUpdateColor()),
		DestroyActionStyle: actionStyle.
			Foreground(cs.GetDestroyColor()),
		FailedActionStyle: actionStyle.
			Foreground(cs.GetFailedColor()),
		NameStyle: colStyle.
			Width(20).
			Align(lg.Center),
		IdStyle: colStyle.
			Width(20).
			Align(lg.Center),
		KindStyle: colStyle.
			Width(20).
			Align(lg.Center),
		ReasonStyle: colStyle.
			MaxWidth(50),
	}
}
