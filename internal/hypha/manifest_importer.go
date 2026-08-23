package hypha

import (
	"embed"
	"fmt"
	"path"

	"github.com/google/go-jsonnet"
)

type EmbedImporter struct {
	FS embed.FS
}

func (e *EmbedImporter) Import(dir, imported string) (jsonnet.Contents, string, error) {
	target := path.Clean(path.Join(dir, imported))
	data, err := e.FS.ReadFile(target)
	if err != nil {
		return jsonnet.Contents{}, "", fmt.Errorf("file `%s` not found: %v", target, err)
	}

	return jsonnet.MakeContents(string(data)), target, nil
}

func (e *EmbedImporter) HasMap() bool {
	return true
}
