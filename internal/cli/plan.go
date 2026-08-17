package cli

import (
	"fmt"
	"strings"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

type PlanSummary struct {
	None      uint64
	Total     uint64
	Created   uint64
	Updated   uint64
	Destroyed uint64
}

func handlePlan(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	filename := args[0]
	specs, err := orc.ParseResourceSpecsFromJsonnet(filename)
	if err != nil {
		return fmt.Errorf("failed to parse resource specs from %s: %v", filename, err)
	}

	for i := range specs {
		orc.AddResource(specs[i])
	}

	const (
		idWidth      = 20
		actionWidth  = 10
		reasonWidth  = 80
		totalWidth   = idWidth + actionWidth + reasonWidth
		totalColumns = 3
		totalSize    = totalWidth + (2 * totalColumns)
	)

	idStyle := lg.NewStyle().
		Width(idWidth).
		Align(lg.Right).
		Foreground(lg.Color("#CECDC3"))

	actionStyle := lg.NewStyle().
		Width(actionWidth).
		Align(lg.Center)

	noActionStyle := actionStyle.
		Foreground(lg.Color(NoOpSymbol.Color))
	createActionStyle := actionStyle.
		Foreground(lg.Color(CreateSymbol.Color))
	updateActionStyle := actionStyle.
		Foreground(lg.Color(UpdateSymbol.Color))
	destroyActionStyle := actionStyle.
		Foreground(lg.Color(DestroySymbol.Color))

	reasonStyle := lg.NewStyle().
		Width(reasonWidth).
		Align(lg.Left).
		Foreground(lg.Color("#CECDC3"))

	orc.Run(hypha.OrchestratorPlanMode)

	borderStyle := lg.NewStyle().
		Foreground(lg.Color("#282726"))
	headerStyle := lg.NewStyle().
		Bold(true).
		Foreground(lg.Color("#CECDC3"))
	headerID := headerStyle.
		Width(idWidth).
		Align(lg.Right).
		Render("ID")
	headerAction := headerStyle.
		Width(actionWidth).
		Align(lg.Center).
		Render("Action")
	headerReason := headerStyle.
		Width(reasonWidth).
		Align(lg.Left).
		Render("Reason")

	fmt.Println()
	fmt.Printf("  %s  %s  %s\n", headerID, headerAction, headerReason)
	fmt.Printf("%s\n", borderStyle.Align(lg.Center).Render(strings.Repeat("─", totalSize)))

	plan := orc.GetPlan()
	if plan == nil {
		return fmt.Errorf("no plans")
	}

	var summary PlanSummary
	plan.VisitPlannedActions(func(idx uint64, action hypha.PlannedAction) bool {
		_ = idx

		var style lg.Style
		var ind string
		switch action.Action {
		case "No":
			style = noActionStyle
			summary.None++
		case "Create":
			style = createActionStyle
			summary.Created++
		case "Update":
			style = updateActionStyle
			summary.Updated++
		case "Destroy":
			style = destroyActionStyle
			summary.Destroyed++
		}

		var act any
		id := idStyle.Render(action.ID)
		reason := reasonStyle.Render(action.Reason)
		if action.Action == "No" {
			act = "None"
		} else {
			act = action.Action
		}

		fmt.Printf("  %s  %s  %s\n", id, style.Render(fmt.Sprintf("%s %s", ind, act)), reason)
		summary.Total++
		return true
	})

	// ╭─────────╮
	// │ Summary │
	// ╰─────────╯
	fmt.Println()
	fmt.Printf("%s\n", borderStyle.Render(strings.Repeat("─", totalSize)))
	fmt.Println()
	printSummary(summary)
	fmt.Println()
	return nil
}

func printSummary(summary PlanSummary) {
	noActionStyle := lg.NewStyle().Foreground(lg.Color(NoOpSymbol.Color))
	createActionStyle := lg.NewStyle().Foreground(lg.Color(CreateSymbol.Color))
	updateActionStyle := lg.NewStyle().Foreground(lg.Color(UpdateSymbol.Color))
	destroyActionStyle := lg.NewStyle().Foreground(lg.Color(DestroySymbol.Color))

	if summary.Created > 0 {
		fmt.Println(createActionStyle.UnsetWidth().Render(fmt.Sprintf("  %s %d/%d Created", CreateSymbol.NF, summary.Created, summary.Total)))
	}

	if summary.Updated > 0 {
		fmt.Println(updateActionStyle.UnsetWidth().Render(fmt.Sprintf("  %s %d/%d Updated", UpdateSymbol.NF, summary.Updated, summary.Total)))
	}

	if summary.Destroyed > 0 {
		fmt.Println(destroyActionStyle.UnsetWidth().Render(fmt.Sprintf("  %s %d/%d Destroyed", DestroySymbol.NF, summary.Destroyed, summary.Total)))
	}

	if summary.None > 0 {
		fmt.Println(noActionStyle.UnsetWidth().Render(fmt.Sprintf("  %s %d/%d No Actions", NoOpSymbol.NF, summary.None, summary.Total)))
	}
}

func init() {
	planCmd := &cobra.Command{
		Use:     "plan",
		Short:   "Preview the pending changes",
		GroupID: "config",
		RunE:    handlePlan,
	}

	RootCmd.AddCommand(planCmd)
}
