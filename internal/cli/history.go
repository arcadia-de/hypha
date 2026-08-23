package cli

import (
	tea "charm.land/bubbletea/v2"
	lg "charm.land/lipgloss/v2"
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
	Style   *HistoryStyle
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

func (m model) View() tea.View {
	var s strings.Builder
	m.Style.PrintTo(m.records, m.cursor, 10, &s)
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
			if rec.Status.GetName() != statusFilter {
				return true // skip
			}
		}

		records = append(records, rec)
		return true
	})

	if len(records) == 0 {
		return nil
	}

	cs := hypha.GetDefaultColorScheme()
	rowStyle := lg.NewStyle().
		MarginLeft(4)
	p := tea.NewProgram(model{
		records: records,
		Style:   NewHistoryStyle(&rowStyle, &cs),
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
