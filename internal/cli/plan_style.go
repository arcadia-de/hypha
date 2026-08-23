package cli

import (
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
)

type PlanStyle struct {
	RowStyle           lg.Style
	HeaderRowStyle     lg.Style
	HeaderNameStyle    lg.Style
	HeaderActionStyle  lg.Style
	HeaderReasonStyle  lg.Style
	NameStyle          lg.Style
	NoActionStyle      lg.Style
	CreateActionStyle  lg.Style
	UpdateActionStyle  lg.Style
	DestroyActionStyle lg.Style
	ReasonStyle        lg.Style
}

func NewPlanStyle(rowStyle *lg.Style, cs *hypha.ColorScheme) *PlanStyle {
	headerStyle := lg.NewStyle().
		Bold(true)
	actionStyle := lg.NewStyle().
		Width(15).
		Padding(0, 1).
		Align(lg.Right)
	return &PlanStyle{
		RowStyle: *rowStyle,
		HeaderRowStyle: rowStyle.
			BorderBottom(true).
			BorderStyle(lg.NormalBorder()).
			BorderForeground(cs.GetBorderColor()),
		HeaderNameStyle: headerStyle.
			Width(20).
			Padding(0, 1).
			Align(lg.Center),
		HeaderActionStyle: headerStyle.
			Width(15).
			Padding(0, 1).
			Align(lg.Right),
		HeaderReasonStyle: headerStyle.
			Width(50).
			Padding(0, 1).
			Align(lg.Left),
		NameStyle: lg.NewStyle().
			Padding(0, 1).
			Width(20).
			Align(lg.Center),
		ReasonStyle: lg.NewStyle().
			Padding(0, 1).
			Width(50).
			Align(lg.Left).
			Foreground(cs.GetMutedColor()),
		NoActionStyle: actionStyle.
			Foreground(cs.GetNoneColor()),
		CreateActionStyle: actionStyle.
			Foreground(cs.GetCreateColor()),
		UpdateActionStyle: actionStyle.
			Foreground(cs.GetUpdateColor()),
		DestroyActionStyle: actionStyle.
			Foreground(cs.GetDestroyColor()),
	}
}

func (style *PlanStyle) PrintRow(value string) {
	fmt.Println(style.RowStyle.Render(value))
}

func (style *PlanStyle) PrintPlanHeaderRow() {
	fmt.Println(style.HeaderRowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		style.HeaderActionStyle.Render("Action"),
		style.HeaderNameStyle.Render("Name"),
		style.HeaderReasonStyle.Render("Reason"),
	)))
}

func (style *PlanStyle) PrintAction(action *hypha.PlannedAction, summary *PlanSummary) {
	var actionStyle lg.Style
	var ind string
	switch action.Action {
	case "No":
		actionStyle = style.NoActionStyle
		summary.None++
	case "Create":
		actionStyle = style.CreateActionStyle
		summary.Created++
	case "Update":
		actionStyle = style.UpdateActionStyle
		summary.Updated++
	case "Destroy":
		actionStyle = style.DestroyActionStyle
		summary.Destroyed++
	}

	var act any
	name := style.NameStyle.Render(action.Name)
	reason := style.ReasonStyle.Render(action.Reason)
	if action.Action == "No" {
		act = "None"
	} else {
		act = action.Action
	}

	style.PrintRow(lg.JoinHorizontal(
		lg.Left,
		actionStyle.Render(fmt.Sprintf("%s %s", ind, act)),
		name,
		reason,
	))
	summary.Total++
}
