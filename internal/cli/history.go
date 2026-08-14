package cli

import (
	tea "charm.land/bubbletea/v2"
	"charm.land/lipgloss/v2"
	"fmt"
	"github.com/spf13/cobra"
	"os"
	"strings"

	"github.com/arcadia-de/hypha/internal/hypha"
)

var kindFilter string
var runidFilter int64
var actionFilter string
var statusFilter string

type model struct {
	records []hypha.HistoryRecord
	cursor  int
}

func (m model) Init() tea.Cmd {
	return nil
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "q", "ctrl+c":
			return m, tea.Quit
		case "up", "k":
			if m.cursor > 0 {
				m.cursor--
			}
		case "down", "j":
			if m.cursor < len(m.records)-1 {
				m.cursor++
			}
		}
	}
	return m, nil
}

const (
	idWidth        = 10
	kindWidth      = 10
	appliedAtWidth = 15
	actionWidth    = 10
	statusWidth    = 10
)

func (m model) View() tea.View {
	var s strings.Builder

	headerStyle := lipgloss.NewStyle().Bold(true).Foreground(lipgloss.Color("#888888"))
	headerID := headerStyle.Width(idWidth).Render("ID")
	headerKind := headerStyle.Width(kindWidth).Render("Kind")
	headerAppliedAt := headerStyle.Width(appliedAtWidth).Render("Applied At")
	headerAction := headerStyle.Width(actionWidth).Render("Action")
	headerStatus := headerStyle.Width(statusWidth).Render("Status")

	s.WriteString(fmt.Sprintf("  %s  %s  %s  %s  %s\n", headerID, headerKind, headerAppliedAt, headerAction, headerStatus))
	s.WriteString(headerStyle.Render(strings.Repeat("─", (idWidth+kindWidth+appliedAtWidth+actionWidth+statusWidth+(2*5)))) + "\n")

	selectedStyle := lipgloss.NewStyle().
		Background(lipgloss.Color("#333333")).
		Bold(true)

	recordStyle := lipgloss.NewStyle()
	recordIDStyle := recordStyle.Width(idWidth)
	recordKindStyle := recordStyle.Width(kindWidth)
	recordAppliedAtStyle := recordStyle.Width(appliedAtWidth)
	recordActionStyle := recordStyle.Width(actionWidth)
	recordStatusStyle := recordStyle.Width(statusWidth)

	for i, record := range m.records {
		recordID := recordIDStyle.Render(record.ID)
		recordKind := recordKindStyle.Render(record.Kind)
		recordAppliedAt := recordAppliedAtStyle.Render(string(record.AppliedAt))
		recordAction := recordActionStyle.Render(record.Action)
		recordStatus := recordStatusStyle.Render(record.Status)

		rowText := fmt.Sprintf("  %s  %s  %s  %s  %s", recordID, recordKind, recordAppliedAt, recordAction, recordStatus)

		if i == m.cursor {
			s.WriteString(selectedStyle.Render(rowText) + "\n")
		} else {
			s.WriteString(rowText + "\n")
		}
	}

	s.WriteString("\n  (Press j/k to navigate • q to quit)\n")

	return tea.View{
		Content: s.String(),
	}
}

func handleHistory(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	var records []hypha.HistoryRecord
	orc.HistoryReplay(func(rec hypha.HistoryRecord) bool {
		if kindFilter != "" {
			if rec.Kind != kindFilter {
				return true // skip
			}
		}

		if runidFilter >= 0 {
			if rec.RunID != uint64(runidFilter) {
				return true // skip
			}
		}

		if actionFilter != "" {
			if rec.Action != actionFilter {
				return true // skip
			}
		}

		if statusFilter != "" {
			if rec.Status != statusFilter {
				return true // skip
			}
		}

		records = append(records, rec)
		return true
	})

	if len(records) == 0 {
		return nil
	}

	p := tea.NewProgram(model{
		records: records,
	})

	if _, err := p.Run(); err != nil {
		fmt.Printf("Error running program: %v\n", err)
		os.Exit(1)
	}

	return nil
}

func init() {
	historyCmd := &cobra.Command{
		Use:     "history",
		Short:   "Show the history of the resource graph",
		GroupID: "inspection",
		RunE:    handleHistory,
	}

	historyCmd.Flags().StringVarP(&kindFilter, "kind", "k", "", "Filter by kind")
	historyCmd.Flags().Int64VarP(&runidFilter, "run-id", "r", -1, "Filter by run id")
	historyCmd.Flags().StringVarP(&actionFilter, "action", "a", "", "Filter by action")
	historyCmd.Flags().StringVarP(&statusFilter, "status", "s", "", "Filter by status")

	RootCmd.AddCommand(historyCmd)
}
