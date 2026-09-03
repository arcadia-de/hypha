package cli

import (
	"os"
	"os/exec"

	tea "charm.land/bubbletea/v2"
)

type EditorFinishedMessage struct {
	Error error
}

func OpenEditor(filename string) tea.Cmd {
	editor := os.Getenv("EDITOR")
	if editor == "" {
		editor = "nano"
	}

	c := exec.Command(editor, filename)
	return tea.ExecProcess(c, func(err error) tea.Msg {
		return EditorFinishedMessage{
			Error: err,
		}
	})
}
