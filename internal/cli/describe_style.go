package cli

import (
	"encoding/json"
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
)

type DescribeStyle struct {
	NameStyle       lg.Style
	RowStyle        lg.Style
	FieldNameStyle  lg.Style
	FieldValueStyle lg.Style
}

func NewDescribeStyle(rowStyle *lg.Style) *DescribeStyle {
	return &DescribeStyle{
		NameStyle: (*rowStyle),
		RowStyle: (*rowStyle).
			PaddingLeft(rowStyle.GetPaddingLeft() + 2),
		FieldNameStyle: lg.NewStyle().
			Align(lg.Left).
			Bold(true).
			Width(18),
		FieldValueStyle: lg.NewStyle(),
	}
}

func (style *DescribeStyle) PrintFieldNameWithRowStyle(rowStyle *lg.Style, name string) {
	fmt.Println((*rowStyle).Render(lg.JoinHorizontal(
		lg.Left,
		style.FieldNameStyle.Render(name+": "),
	)))
}

func (style *DescribeStyle) PrintFieldWithRowAndNameStyle(rowStyle *lg.Style, nameStyle *lg.Style, name string, value string) {
	fmt.Println((*rowStyle).Render(lg.JoinHorizontal(
		lg.Left,
		nameStyle.Render(name+": "),
		style.FieldValueStyle.Render(value),
	)))
}

func (style *DescribeStyle) PrintFieldWithRowStyle(rowStyle *lg.Style, name string, value string) {
	fmt.Println((*rowStyle).Render(lg.JoinHorizontal(
		lg.Left,
		style.FieldNameStyle.Render(name+": "),
		style.FieldValueStyle.Render(value),
	)))
}

func (style *DescribeStyle) PrintField(name string, value string) {
	style.PrintFieldWithRowStyle(&style.RowStyle, name, value)
}

func (style *DescribeStyle) PrintAnnotations(annotations []hypha.ResourceAnnotation) {
	style.PrintFieldNameWithRowStyle(&style.RowStyle, "Annotations")
	rowStyle := style.RowStyle.PaddingLeft(style.RowStyle.GetPaddingLeft() + 2)
	for _, annotation := range annotations {
		fmt.Println(rowStyle.Render(lg.JoinHorizontal(
			lg.Left,
			style.FieldValueStyle.Render(fmt.Sprintf("- %s=%s", annotation.Key, annotation.Value)),
		)))
	}
}

func (style *DescribeStyle) PrintLabels(labels []string) {
	style.PrintFieldNameWithRowStyle(&style.RowStyle, "Labels")
	rowStyle := style.RowStyle.PaddingLeft(style.RowStyle.GetPaddingLeft() + 2)
	for _, label := range labels {
		fmt.Println(rowStyle.Render(lg.JoinHorizontal(
			lg.Left,
			style.FieldValueStyle.Render(fmt.Sprintf("- %s", label)),
		)))
	}
}

func (style *DescribeStyle) PrintStatus(status hypha.ResourceStatus) {
	style.PrintFieldNameWithRowStyle(&style.RowStyle, "Status")
	rowStyle := style.RowStyle.PaddingLeft(style.RowStyle.GetPaddingLeft() + 2)
	nameStyle := style.FieldNameStyle.Width(style.FieldNameStyle.GetWidth() - 2)
	style.PrintFieldWithRowAndNameStyle(&rowStyle, &nameStyle, "State", status.State.String())
	style.PrintFieldWithRowAndNameStyle(&rowStyle, &nameStyle, "Action", hypha.GetControllerActionName(status.Action))
	style.PrintFieldWithRowAndNameStyle(&rowStyle, &nameStyle, "Reason", status.Reason)
}

func (style *DescribeStyle) PrintSpec(spec any) {
	style.PrintFieldNameWithRowStyle(&style.RowStyle, "Spec")
	rowStyle := style.RowStyle.PaddingLeft(style.RowStyle.GetPaddingLeft() + 2)
	bytes, err := json.MarshalIndent(spec, "", "  ")
	if err != nil {
		fmt.Println(rowStyle.Render(fmt.Sprintf("cannot marshal spec: %v", err)))
		return
	}

	fmt.Println(rowStyle.Render(string(bytes)))
}

func (style *DescribeStyle) Print(resource hypha.Resource) {
	fmt.Println(style.NameStyle.Render(resource.Metadata.Name))

	style.PrintField("Kind", resource.Kind)
	style.PrintField("ID", resource.ID)
	style.PrintField("Namespace", resource.Metadata.Namespace)

	if resource.HasLabels() {
		style.PrintLabels(resource.Metadata.Labels)
	}

	if resource.HasAnnotations() {
		style.PrintAnnotations(resource.Metadata.Annotations)
	}

	fmt.Println()

	style.PrintSpec(resource.Spec)

	fmt.Println()

	style.PrintStatus(resource.Status)
}
