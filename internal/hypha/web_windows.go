package hypha

import (
	"os/exec"
)

func CreateOpenCommand(url string) *exec.Cmd {
	return exec.Command("rundll32", "url.dll,FileProtocolHandler", url)
}
