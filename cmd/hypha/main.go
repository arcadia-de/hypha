package main

import (
	"charm.land/log/v2"
	"github.com/arcadia-de/hypha/internal/cli"

	"os"
)

func main() {
	if err := cli.Execute(); err != nil {
		log.Fatal(err)
		os.Exit(1)
	}
}
