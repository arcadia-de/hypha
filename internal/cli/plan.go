package cli

import (
	"fmt"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func HandlePlan(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	cs := hypha.GetDefaultColorScheme()
	const (
		idWidth      = 20
		actionWidth  = 10
		reasonWidth  = 80
		totalWidth   = idWidth + actionWidth + reasonWidth
		totalColumns = 3
		totalSize    = totalWidth + (2 * totalColumns)
	)

	rowStyle := lg.NewStyle().
		MarginLeft(2)
	summaryStyle := NewPlanSummaryStyle(&rowStyle, &cs)
	planStyle := NewPlanStyle(&rowStyle, &cs)

	if err := orc.ProcessDiscoveredManifests(); err != nil {
		return fmt.Errorf("failed to process discovered manifests: %v", err)
	}

	info := hypha.RunInfo{
		Mode: hypha.RunPlanMode,
	}
	fmt.Printf("mode: %d\n", info.Mode)
	orc.Run(info)
	fmt.Println()

	vlog := orc.GetValidationLog()
	if !vlog.IsEmpty() {
		vlog.Print(cs)
	}

	var summary PlanSummary
	plan := orc.GetPlan()

	if plan != nil {
		fmt.Println()
		planStyle.PrintPlanHeaderRow()
		plan.VisitPlannedActions(func(idx uint64, action hypha.PlannedAction) bool {
			_ = idx
			planStyle.PrintAction(&action, &summary)
			return true
		})
	}

	if summary.Total > 0 {
		summaryStyle.Print(&summary)
	}

	return nil
}

func init() {
	planCmd := &cobra.Command{
		Use:     "plan",
		Short:   "Preview the pending changes",
		GroupID: "config",
		RunE:    HandlePlan,
	}

	RootCmd.AddCommand(planCmd)
}
