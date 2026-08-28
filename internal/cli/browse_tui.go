package cli

import (
	"fmt"
	"log"
	"strings"

	tea "charm.land/bubbletea/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
)

type Model struct {
	Tree   *hypha.Tree
	Cursor int
}

type FlatNode struct {
	Node  *hypha.Node
	Depth int
}

func (m Model) Init() tea.Cmd {
	return nil
}

func (m Model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	flatList := m.flatten()
	maxIndex := len(flatList) - 1

	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "ctrl+c", "q":
			return m, tea.Quit

		case "up", "k":
			if m.Cursor > 0 {
				m.Cursor--
			}

		case "down", "j":
			if m.Cursor < maxIndex {
				m.Cursor++
			}

		case "left":
			if m.Cursor >= 0 && m.Cursor <= maxIndex {
				flatList[m.Cursor].Node.Expanded = false
			}

		case "right":
			if m.Cursor >= 0 && m.Cursor <= maxIndex {
				flatList[m.Cursor].Node.Expanded = true
			}

		case "enter", "space":
			if m.Cursor >= 0 && m.Cursor <= maxIndex {
				target := flatList[m.Cursor].Node
				if len(target.Children) > 0 {
					target.Expanded = !target.Expanded
				}
			}
		}
	}
	return m, nil
}

func (m Model) View() tea.View {
	flatList := m.flatten()
	var s strings.Builder

	if len(flatList) == 0 {
		return tea.NewView(s.String() + " No resources found.\n")
	}

	for i, fn := range flatList {
		cursorSign := "  "
		if m.Cursor == i {
			cursorSign = "➔ "
		}

		indent := strings.Repeat("    ", fn.Depth)
		icon := "• "
		if len(fn.Node.Children) > 0 {
			if fn.Node.Expanded {
				icon = "▼ "
			} else {
				icon = "▶ "
			}
		}

		const stateColor = "\033[37m"
		lineText := fmt.Sprintf("%s%s%s%s%s\033[0m", indent, icon, stateColor, fn.Node.Name, "X")
		if m.Cursor == i {
			s.WriteString(fmt.Sprintf("%s\033[1;7m%s\033[0m\n", cursorSign, lineText))
		} else {
			s.WriteString(fmt.Sprintf("%s%s\n", cursorSign, lineText))
		}
	}

	return tea.NewView(s.String())
}

func (m Model) flatten() []FlatNode {
	var result []FlatNode
	var traverse func(n *hypha.Node, depth int)

	traverse = func(n *hypha.Node, depth int) {
		if n == nil {
			return
		}
		result = append(result, FlatNode{Node: n, Depth: depth})
		if n.Expanded {
			for _, child := range n.Children {
				traverse(child, depth+1)
			}
		}
	}

	for _, rootNode := range m.Tree.Nodes {
		traverse(rootNode, 0)
	}

	return result
}

func HandleBrowseTui() error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	if err := orc.ProcessDiscoveredManifests(); err != nil {
		return fmt.Errorf("failed to process discovered manifests: %v", err)
	}

	rg := orc.GetResourceGraph()
	if !rg.IsEmpty() {
		selector := hypha.NewNegateSelector(hypha.NewOrResourceSelector([]hypha.ResourceSelector{
			hypha.NewKindResourceSelector("Controller"),
			hypha.NewKindResourceSelector("PackageManager"),
		}))
		resources := rg.ListResourcesWithSelector(selector)
		tree := hypha.NewTree(resources)
		model := Model{
			Tree:   tree,
			Cursor: 0,
		}

		p := tea.NewProgram(model)
		if _, err := p.Run(); err != nil {
			log.Fatalf("Error running program: %v", err)
		}
	}

	return nil
}
