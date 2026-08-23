package cli

import (
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
)

type DescribeStyle struct {
	RowStyle lg.Style

	FieldNameStyle  lg.Style
	FieldValueStyle lg.Style
}

func NewDescribeStyle(rowStyle *lg.Style) *DescribeStyle {
	return &DescribeStyle{
		RowStyle: (*rowStyle),
		FieldNameStyle: lg.NewStyle().
			Align(lg.Right).
			Bold(true).
			Width(15),
		FieldValueStyle: lg.NewStyle(),
	}
}

func (style *DescribeStyle) PrintFieldNameWithRowStyle(rowStyle *lg.Style, name string) {
	fmt.Println((*rowStyle).Render(lg.JoinHorizontal(
		lg.Left,
		style.FieldNameStyle.Render(name),
		": ",
	)))
}

func (style *DescribeStyle) PrintFieldWithRowStyle(rowStyle *lg.Style, name string, value string) {
	fmt.Println((*rowStyle).Render(lg.JoinHorizontal(
		lg.Left,
		style.FieldNameStyle.Render(name),
		": ",
		style.FieldValueStyle.Render(value),
	)))
}

func (style *DescribeStyle) PrintField(name string, value string) {
	style.PrintFieldWithRowStyle(&style.RowStyle, name, value)
}

func (style *DescribeStyle) PrintAnnotations(annotations []hypha.ResourceAnnotation) {
	style.PrintFieldNameWithRowStyle(&style.RowStyle, "Annotations")
	rowStyle := style.RowStyle.MarginLeft(style.RowStyle.GetMarginLeft() + 2 + style.FieldNameStyle.GetWidth())
	for _, annotation := range annotations {
		fmt.Println(rowStyle.Render(lg.JoinHorizontal(
			lg.Left,
			style.FieldValueStyle.Render(fmt.Sprintf("%s=%s", annotation.Key, annotation.Value)),
		)))
	}
}

func (style *DescribeStyle) PrintLabels(labels []string) {
	style.PrintFieldNameWithRowStyle(&style.RowStyle, "Labels")
	rowStyle := style.RowStyle.MarginLeft(style.RowStyle.GetMarginLeft() + 2 + style.FieldNameStyle.GetWidth())
	for _, label := range labels {
		fmt.Println(rowStyle.Render(lg.JoinHorizontal(
			lg.Left,
			style.FieldValueStyle.Render(label),
		)))
	}
}

func (style *DescribeStyle) PrintStatus(status hypha.ResourceStatus) {
	fmt.Println()
	style.PrintFieldNameWithRowStyle(&style.RowStyle, "Status")
	rowStyle := style.RowStyle.MarginLeft(style.RowStyle.GetMarginLeft() + 2)
	style.PrintFieldWithRowStyle(&rowStyle, "State", status.State.String())
	style.PrintFieldWithRowStyle(&rowStyle, "Action", hypha.GetControllerActionName(status.Action))
	style.PrintFieldWithRowStyle(&rowStyle, "Reason", status.Reason)
}

func (style *DescribeStyle) Print(resource hypha.Resource) {
	fmt.Println(style.RowStyle.Render(resource.Metadata.Name))
	style.PrintField("ID", resource.ID)
	style.PrintField("Kind", resource.Kind)

	if resource.HasLabels() {
		style.PrintLabels(resource.Metadata.Labels)
	}

	if resource.HasAnnotations() {
		style.PrintAnnotations(resource.Metadata.Annotations)
	}

	if len(resource.Spec) > 0 {
		style.PrintField("Spec", resource.Spec)
	}

	style.PrintStatus(resource.Status)
}
