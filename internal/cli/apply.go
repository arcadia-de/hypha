package cli

import (
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

type ApplySummary struct {
	Total     uint64
	NoOp      uint64
	Created   uint64
	Updated   uint64
	Destroyed uint64
	Failed    uint64
}

func PrintAppliedSummary(summary ApplySummary, rowStyle *lg.Style, cs *hypha.ColorScheme) {
	if summary.Total == 0 {
		return
	}

	fmt.Println()

	actionStyle := lg.NewStyle()
	createdStyle := actionStyle.
		Foreground(cs.GetCreateColor())
	noopStyle := actionStyle.
		Foreground(cs.GetNoneColor())
	updatedStyle := actionStyle.
		Foreground(cs.GetUpdateColor())
	destroyedStyle := actionStyle.
		Foreground(cs.GetDestroyColor())
	failedStyle := actionStyle.
		Foreground(cs.GetFailedColor())

	fmt.Println(rowStyle.Render(fmt.Sprintf("Applied %d changes", summary.Total)))
	if summary.Created > 0 {
		fmt.Println(rowStyle.Render(createdStyle.Render(fmt.Sprintf("%s %d/%d Created", CreateSymbol.NF, summary.Created, summary.Total))))
	}

	if summary.NoOp > 0 {
		fmt.Println(rowStyle.Render(noopStyle.Render(fmt.Sprintf("%s %d/%d Unchanged", NoOpSymbol.NF, summary.NoOp, summary.Total))))
	}

	if summary.Updated > 0 {
		fmt.Println(rowStyle.Render(updatedStyle.Render(fmt.Sprintf("%s %d/%d Updated", UpdateSymbol.NF, summary.Updated, summary.Total))))
	}

	if summary.Destroyed > 0 {
		fmt.Println(rowStyle.Render(destroyedStyle.Render(fmt.Sprintf("%s %d/%d Destroyed", DestroySymbol.NF, summary.Destroyed, summary.Total))))
	}

	if summary.Failed > 0 {
		fmt.Println(rowStyle.Render(failedStyle.Render(fmt.Sprintf("%s %d/%d Failed", DestroySymbol.NF, summary.Failed, summary.Total))))
	}
}

func PrintAppliedActions(orc *hypha.Orchestrator, rowStyle *lg.Style, cs *hypha.ColorScheme) {
	style := NewApplyStyle(rowStyle, cs)
	fmt.Println()
	fmt.Println(style.HeaderRowStyle.Render(lg.JoinHorizontal(
		lg.Left,
		style.HeaderActionStyle.Render("Action"),
		style.HeaderKindStyle.Render("Kind"),
		style.HeaderNameStyle.Render("Name"),
		style.HeaderReasonStyle.Render("Reason"),
	)))

	var summary ApplySummary
	orc.VisitAppliedActions(func(idx uint64, action hypha.AppliedAction) bool {
		act := hypha.GetControllerActionName(hypha.ControllerAction(action.Action))
		switch act {
		case "No Action":
			act = style.NoActionStyle.Render(act)
			summary.NoOp++
		case "Create":
			act = style.CreateActionStyle.Render(act)
			summary.Created++
		case "Update":
			act = style.UpdateActionStyle.Render(act)
			summary.Updated++
		case "Destroy":
			act = style.DestroyActionStyle.Render(act)
			summary.Destroyed++
		}

		fmt.Println(rowStyle.Render(lg.JoinHorizontal(
			lg.Left,
			act,
			style.KindStyle.Render(action.Kind),
			style.NameStyle.Render(action.Name),
			style.ReasonStyle.Render(action.Reason),
		)))
		summary.Total++
		return true
	})

	PrintAppliedSummary(summary, rowStyle, cs)
}

func handleApply(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	if err := orc.ProcessDiscoveredManifests(); err != nil {
		return fmt.Errorf("failed to process discovered manifests: %v", err)
	}

	err = orc.Run(hypha.OrchestratorApplyMode)
	if err != nil {
		return fmt.Errorf("failed to run Orchestrator: %v", err)
	}

	cs := hypha.GetDefaultColorScheme()
	vlog := orc.GetValidationLog()
	if !vlog.IsEmpty() {
		vlog.Print(cs)
	}

	rowStyle := lg.NewStyle().
		MarginLeft(2)
	PrintAppliedActions(orc, &rowStyle, &cs)
	fmt.Println()

	err = orc.PrintMetricsIfDesired()
	if err != nil {
		return fmt.Errorf("failed to print metrics: %v", err)
	}

	return nil
}

func init() {
	applyCmd := &cobra.Command{
		Use:     "apply",
		Short:   "Apply your configuration",
		GroupID: "config",
		RunE:    handleApply,
	}
	applyCmd.Flags().BoolP("print-telemetry", "", false, "Enable telemetry")
	applyCmd.Flags().BoolP("dry-run", "", false, "Don't actually do anything, just log")

	RootCmd.AddCommand(applyCmd)
}
