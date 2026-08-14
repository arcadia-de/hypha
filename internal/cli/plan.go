package cli

import (
	"fmt"
	"strings"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

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

	idStyle := lg.NewStyle().Width(idWidth).Align(lg.Right)
	actionStyle := lg.NewStyle().Width(actionWidth).Align(lg.Center)
	reasonStyle := lg.NewStyle().Width(reasonWidth).Align(lg.Left)

	noActionStyle := actionStyle.Foreground(lg.Color("#9E9E9E"))
	createActionStyle := actionStyle.Foreground(lg.Color("#4CAF50"))
	updateActionStyle := actionStyle.Foreground(lg.Color("#2196F3"))
	destroyActionStyle := actionStyle.Foreground(lg.Color("#F44336"))

	orc.Run(hypha.OrchestratorPlanMode)

	headerStyle := lg.NewStyle().Bold(true).Foreground(lg.Color("#888888"))
	headerID := headerStyle.Width(idWidth).Align(lg.Right).Render("ID")
	headerAction := headerStyle.Width(actionWidth).Align(lg.Center).Render("Action")
	headerReason := headerStyle.Width(reasonWidth).Align(lg.Left).Render("Reason")

	fmt.Println()
	fmt.Printf("  %s  %s  %s\n", headerID, headerAction, headerReason)
	fmt.Printf("%s\n", headerStyle.Align(lg.Center).Render(strings.Repeat("─", totalSize)))

	const (
		noActionInd      = "󰅚"
		createActionInd  = "󰐕"
		updateActionInd  = "󰏫"
		destroyActionInd = "󰩹"
	)

	var (
		none      = 0
		total     = 0
		created   = 0
		updated   = 0
		destroyed = 0
	)

	orc.VisitPlannedActions(func(action hypha.PlannedAction) bool {
		var style lg.Style
		var ind string
		switch action.Action {
		case "No":
			style = noActionStyle
			none++
		case "Create":
			style = createActionStyle
			created++
		case "Update":
			style = updateActionStyle
			updated++
		case "Destroy":
			style = destroyActionStyle
			destroyed++
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
		total++
		return true
	})
	fmt.Printf("%s\n", headerStyle.Render(strings.Repeat("─", totalSize)))
	fmt.Println()

	if none > 0 {
		fmt.Println(noActionStyle.UnsetWidth().Render(fmt.Sprintf("  %s %d/%d No Actions", noActionInd, none, total)))
	}

	if created > 0 {
		fmt.Println(createActionStyle.UnsetWidth().Render(fmt.Sprintf("  %s %d/%d Created", createActionInd, created, total)))
	}

	if updated > 0 {
		fmt.Println(updateActionStyle.UnsetWidth().Render(fmt.Sprintf("  %s %d/%d Updated", updateActionInd, updated, total)))
	}

	if destroyed > 0 {
		fmt.Println(destroyActionStyle.UnsetWidth().Render(fmt.Sprintf("  %s %d/%d Destroyed", destroyActionInd, destroyed, total)))
	}

	fmt.Println()

	return nil
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
