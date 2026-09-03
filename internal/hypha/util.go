package hypha

/*
#cgo pkg-config: hypha-uninstalled
*/
import "C"

import (
	"errors"
	"os"
	"strings"
)

func Capitalize(s string) string {
	if s == "" {
		return ""
	}

	return strings.ToUpper(s[:1]) + s[1:]
}

func FileExists(filename string) bool {
	_, err := os.Stat(filename)
	if err == nil {
		return true
	}

	if errors.Is(err, os.ErrNotExist) {
		return false
	}

	return false
}
