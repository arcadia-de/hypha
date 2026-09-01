package cli

import (
	"fmt"
	"io"
	"strings"
	"text/template"
)

type TldrExample struct {
	Desc string
	Ex   string
}

type TldrContext struct {
	Title       string
	Description string
	Docs        string
	Command     string
	Examples    []TldrExample
}

func GenTldrPage(out io.Writer) error {
	sb := strings.Builder{}
	fmt.Fprintln(&sb, "# {{.Title}}")
	fmt.Fprintln(&sb)
	fmt.Fprintln(&sb, "> {{.Description}}")
	fmt.Fprintln(&sb, "> More Information: {{.Docs}}")
	fmt.Fprintln(&sb)
	fmt.Fprintln(&sb, "{{range .Examples}}")
	fmt.Fprintln(&sb, "- {{ .Desc }}")
	fmt.Fprintln(&sb, "`{{$.Command }} {{ .Ex }}`")
	fmt.Fprintln(&sb, "{{end}}")

	ctx := TldrContext{
		Title:       "Hypha",
		Description: "A dotfiles manager",
		Docs:        "https://github.com/arcadia-de/hypha/wiki",
		Command:     "hypha",
		Examples: []TldrExample{
			{
				Desc: "List of all resources",
				Ex:   "list",
			},
		},
	}

	tpl := template.Must(template.New("tldr").Parse(sb.String()))
	return tpl.Execute(out, ctx)
}
