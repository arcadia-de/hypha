package schema

import (
	_ "embed"
)

//go:embed hypha.schema.json
var ManifestSchemaJson []byte
