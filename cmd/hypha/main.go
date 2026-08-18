package main

import (
	"charm.land/log/v2"
	"github.com/arcadia-de/hypha/internal/cli"

	"os"
	"runtime"
)

func main() {
	runtime.LockOSThread()
	if err := cli.Execute(); err != nil {
		log.Fatal(err)
		os.Exit(1)
	}
}
