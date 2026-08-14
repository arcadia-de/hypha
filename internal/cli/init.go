package cli

import (
	"fmt"
	"github.com/spf13/cobra"
	"os"
	"text/template"

	"github.com/arcadia-de/hypha/internal/hypha"
)

type TemplateContext interface {
	GetFilename() string
}

func WriteTemplate[C TemplateContext](tpl string, ctx C) error {
	flags := os.O_RDWR | os.O_CREATE
	file, err := os.OpenFile(ctx.GetFilename(), flags, 0644)
	if err != nil {
		return err
	}

	defer file.Close()
	template := template.Must(template.New(fmt.Sprintf("%s.tpl", ctx.GetFilename())).Parse(tpl))
	fmt.Printf("generating %s....", ctx.GetFilename())
	return template.Execute(file, ctx)
}

type GenLuaContext struct {
	Filename    string
	Expressions []string
}

func (ctx GenLuaContext) GetFilename() string {
	return ctx.Filename
}

const luaTpl = `
{{- range .Expressions -}}
{{ . }}
{{- end -}}
`

func GenLua(ctx GenLuaContext) error {
	return WriteTemplate(luaTpl, ctx)
}

type GenJsonnetContext struct {
	Filename string
}

func (ctx GenJsonnetContext) GetFilename() string {
	return ctx.Filename
}

const mainJsonnetTpl = `
local shared = import "shared_config";
[
	// TODO: Declare your manifests here
	shared.Package("git") + 
		shared.Labels([
			"test",
			"example",
		]),
]`

func GenJsonnet(ctx GenJsonnetContext) error {
	return WriteTemplate(mainJsonnetTpl, ctx)
}

func handleInit(cmd *cobra.Command, args []string) error {
	config_dir, err := hypha.EnsureConfigDirExists()
	genInitCtx := GenLuaContext{
		Filename: fmt.Sprintf("%s/init.lua", config_dir),
		Expressions: []string{
			"print('Hello World')",
		},
	}
	err = GenLua(genInitCtx)
	if err != nil {
		return fmt.Errorf("failed to generate init.lua: %v", err)
	}

	genMainJsonnetCtx := GenJsonnetContext{
		Filename: fmt.Sprintf("%s/main.jsonnet", config_dir),
	}
	err = GenJsonnet(genMainJsonnetCtx)
	if err != nil {
		return fmt.Errorf("failed to generate main.jsonnet: %v", err)
	}

	return nil
}

var initCmd = &cobra.Command{
	Use:     "init",
	Short:   "Initialize hypha on a system",
	GroupID: "config",
	RunE:    handleInit,
}

func init() {
	RootCmd.AddCommand(initCmd)
}
