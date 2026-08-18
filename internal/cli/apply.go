package cli

import (
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

type AppliedSummary struct {
	Total     uint64
	NoOp      uint64
	Created   uint64
	Failed    uint64
	Destroyed uint64
	Updated   uint64
}

func PrintAppliedSummary(summary AppliedSummary) {
	fmt.Println()

	totalStyle := lg.NewStyle()

	createdStyle := lg.NewStyle().
		Foreground(lg.Color(CreateSymbol.Color)).
		PaddingLeft(2)

	noopStyle := lg.NewStyle().
		Foreground(lg.Color(NoOpSymbol.Color)).
		PaddingLeft(2)

	updatedStyle := lg.NewStyle().
		Foreground(lg.Color(UpdateSymbol.Color)).
		PaddingLeft(2)

	destroyedStyle := lg.NewStyle().
		Foreground(lg.Color(DestroySymbol.Color)).
		PaddingLeft(2)

	failedStyle := lg.NewStyle().
		Foreground(lg.Color(DestroySymbol.Color)).
		PaddingLeft(2)

	textStyle := lg.NewStyle().
		Foreground(lg.Color("#CECDC3"))

	num_total := totalStyle.Render(fmt.Sprintf("%d", summary.Total))
	fmt.Println(textStyle.Render(fmt.Sprintf("Applied %s changes:\n", num_total)))
	fmt.Println(createdStyle.Render(fmt.Sprintf("%s %d Created", CreateSymbol.NF, summary.Created)))
	fmt.Println(noopStyle.Render(fmt.Sprintf("%s %d Unchanged", NoOpSymbol.NF, summary.NoOp)))
	fmt.Println(updatedStyle.Render(fmt.Sprintf("%s %d Updated", UpdateSymbol.NF, summary.Updated)))
	fmt.Println(destroyedStyle.Render(fmt.Sprintf("%s %d Destroyed", DestroySymbol.NF, summary.Failed)))
	fmt.Println(failedStyle.Render(fmt.Sprintf("%s %d Failed", DestroySymbol.NF, summary.Failed)))
}

func PrintAppliedActions(orc *hypha.Orchestrator) {
	const (
		idWidth      = 20
		reasonWidth  = 80
		totalWidth   = idWidth + actionWidth + reasonWidth
		totalColumns = 3
		totalSize    = totalWidth + (2 * totalColumns)

		padding = 2
	)

	actionStyle := lg.NewStyle().
		Width(8 + (padding * 2)).
		Align(lg.Center).
		PaddingLeft(padding).
		PaddingRight(padding)

	noActionStyle := actionStyle.
		Foreground(lg.Color(NoOpSymbol.Color))
	createActionStyle := actionStyle.
		Foreground(lg.Color(CreateSymbol.Color))
	updateActionStyle := actionStyle.
		Foreground(lg.Color(UpdateSymbol.Color))
	destroyActionStyle := actionStyle.
		Foreground(lg.Color(DestroySymbol.Color))

	idxStyle := lg.NewStyle().
		Width(4).
		Align(lg.Right).
		Foreground(lg.Color("#403E3C"))

	idStyle := lg.NewStyle().
		Width(20).
		Align(lg.Center).
		Foreground(lg.Color("#CECDC3"))

	kindStyle := lg.NewStyle().
		Width(8).
		Align(lg.Center).
		Foreground(lg.Color("#CECDC3"))

	reasonStyle := lg.NewStyle().
		Foreground(lg.Color("#575653"))

	var (
		total     uint64 = 0
		noop      uint64 = 0
		created   uint64 = 0
		updated   uint64 = 0
		destroyed uint64 = 0
		failed    uint64 = 0
	)

	fmt.Println()
	orc.VisitAppliedActions(func(idx uint64, action hypha.AppliedAction) bool {
		act := hypha.GetControllerActionName(hypha.ControllerAction(action.Action))
		num := idxStyle.Render(fmt.Sprintf("%d", idx))

		switch act {
		case "None":
			act = noActionStyle.Render(act)
			noop++
		case "Create":
			act = createActionStyle.Render(act)
			created++
		case "Update":
			act = updateActionStyle.Render(act)
			updated++
		case "Destroy":
			act = destroyActionStyle.Render(act)
			destroyed++
		}

		kind := kindStyle.Render("Kind")
		id := idStyle.Render(action.ID)
		reason := reasonStyle.Render(fmt.Sprintf("(%s)", action.Reason))
		if action.Reason != "" {
			fmt.Printf(" %s  %s  %s  %s %s\n", num, act, kind, id, reason)
		} else {
			fmt.Printf(" %s  %s  %s  %s \n", num, act, kind, id)
		}

		total++
		return true
	})

	if total > 0 {
		PrintAppliedSummary(AppliedSummary{
			Total:     total,
			NoOp:      noop,
			Created:   created,
			Updated:   updated,
			Destroyed: destroyed,
			Failed:    failed,
		})
	}

	fmt.Println()
}

func handleApply(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	orc.ProcessDiscoveredManifests()

	err = orc.Run(hypha.OrchestratorApplyMode)
	if err != nil {
		return fmt.Errorf("failed to run Orchestrator: %v", err)
	}

	PrintAppliedActions(orc)

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
