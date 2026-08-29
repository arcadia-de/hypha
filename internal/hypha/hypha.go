package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include "hypha.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"os"
	"strings"
	"unsafe"

	"github.com/spf13/viper"

	"errors"
)

func EnsureStateDirExists() (string, error) {
	dir := viper.GetString("state-dir")
	err := os.MkdirAll(dir, 0755)
	if err != nil {
		return "", fmt.Errorf("failed to create state dir '%s': %v", dir, err)
	}

	return dir, nil
}

func EnsureConfigDirExists() (string, error) {
	config_dir := viper.GetString("config-dir")

	info, err := os.Lstat(config_dir)
	if err != nil {
		return "", fmt.Errorf("Error checking directory (e.g., permission denied): %v\n", err)
	} else if info.IsDir() || info.Mode()&os.ModeSymlink != 0 {
		return config_dir, nil
	} else if !errors.Is(err, os.ErrNotExist) {
		return "", fmt.Errorf("path `%s` exists, but is not a directory", config_dir)
	}

	err = os.MkdirAll(config_dir, 0755)
	if err != nil {
		return "", fmt.Errorf("failed to create configuration dir '%s': %v", config_dir, err)
	}

	return config_dir, nil
}

func EnsureCacheDirExists() (string, error) {
	dir := viper.GetString("cache-dir")
	err := os.MkdirAll(dir, 0755)
	if err != nil {
		return "", fmt.Errorf("failed to create cache dir '%s': %v", dir, err)
	}

	return dir, nil
}

func InitHypha(luarocksDir string) {
	libs := []string{
		"$HOME/.local/state/hypha/?.lua",
		"$HOME/.local/state/hypha/lua/?.lua",
		"$XDG_CONFIG_HOME/hypha/?.lua",
		"$XDG_CONFIG_HOME/hypha/lua/?.lua",
	}
	expandedPath := os.ExpandEnv(strings.Join(libs, ";") + ";;")
	os.Setenv("LUA_PATH", expandedPath)

	cLuarocksDir := C.CString(luarocksDir)
	defer C.free(unsafe.Pointer(cLuarocksDir))

	C.InitHypha(cLuarocksDir)
}
