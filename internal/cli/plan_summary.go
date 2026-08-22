package cli

import (
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
)

type PlanSummaryStyle struct {
	RowStyle       lg.Style
	CreatedStyle   lg.Style
	UpdatedStyle   lg.Style
	DestroyedStyle lg.Style
	NoneStyle      lg.Style
}

func NewPlanSummaryStyle(rowStyle *lg.Style, cs *hypha.ColorScheme) *PlanSummaryStyle {
	return &PlanSummaryStyle{
		RowStyle: *rowStyle,
		CreatedStyle: lg.NewStyle().
			Foreground(cs.GetCreateColor()),
		UpdatedStyle: lg.NewStyle().
			Foreground(cs.GetUpdateColor()),
		DestroyedStyle: lg.NewStyle().
			Foreground(cs.GetDestroyColor()),
		NoneStyle: lg.NewStyle().
			Foreground(cs.GetNoneColor()),
	}
}

func (style *PlanSummaryStyle) Print(ps *PlanSummary) {
	fmt.Println()
	if ps.Created > 0 {
		style.PrintRow(style.CreatedStyle.Render(fmt.Sprintf("%s %d/%d Created", CreateSymbol.NF, ps.Created, ps.Total)))
	}

	if ps.Updated > 0 {
		style.PrintRow(style.CreatedStyle.Render(fmt.Sprintf("%s %d/%d Updated", UpdateSymbol.NF, ps.Updated, ps.Total)))
	}

	if ps.Destroyed > 0 {
		style.PrintRow(style.DestroyedStyle.Render(fmt.Sprintf("  %s %d/%d Destroyed", DestroySymbol.NF, ps.Destroyed, ps.Total)))
	}

	if ps.None > 0 {
		style.PrintRow(style.NoneStyle.Render(fmt.Sprintf("  %s %d/%d No Actions", NoOpSymbol.NF, ps.None, ps.Total)))
	}
	fmt.Println()
}

func (style *PlanSummaryStyle) PrintRow(value string) {
	fmt.Println(style.RowStyle.Render(value))
}

type PlanSummary struct {
	None      uint64
	Total     uint64
	Created   uint64
	Updated   uint64
	Destroyed uint64
}
