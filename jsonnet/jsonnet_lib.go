package jsonnet

import (
	"embed"
)

//go:embed lib/*
var LibsonnetFiles embed.FS
