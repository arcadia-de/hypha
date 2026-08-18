package hypha

/*
#cgo pkg-config: hypha-uninstalled
*/
import "C"

import (
	"strings"
)

func Capitalize(s string) string {
	if s == "" {
		return ""
	}

	return strings.ToUpper(s[:1]) + s[1:]
}

//export goPrintRuntimeInfo
func goPrintRuntimeInfo() {
	//TODO(@s0cks): implement
}
