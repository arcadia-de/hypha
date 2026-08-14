package main

import (
	"charm.land/log/v2"
	"github.com/arcadia-de/hypha/internal/cli"
	"github.com/arcadia-de/hypha/internal/hypha"

	"os"
)

func main() {
	hypha.InitHypha()
	if err := cli.Execute(); err != nil {
		log.Fatal(err)
		os.Exit(1)
	}
}
