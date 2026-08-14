package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include "hypha.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"github.com/spf13/viper"
	"os"
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
	err := os.MkdirAll(config_dir, 0755)
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

func InitHypha() {
	C.InitHypha()
}
