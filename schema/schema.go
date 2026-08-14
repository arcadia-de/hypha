package schema

import (
	_ "embed"
)

const (
	ManifestSchemaId = "https://github.com/arcadia-de/hypha"
)

//go:embed hypha.schema.json
var ManifestSchemaJson []byte
