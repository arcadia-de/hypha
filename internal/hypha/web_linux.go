package hypha

import (
	"os/exec"
)

func CreateOpenCommand(url string) *exec.Cmd {
	return exec.Command("xdg-open", url)
}
