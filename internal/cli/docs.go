package cli

import (
	"fmt"
	"strings"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func GetDocsAnnotation(res hypha.Resource) *hypha.ResourceAnnotation {
	for _, annotation := range res.Metadata.Annotations {
		if strings.HasPrefix(annotation.Key, "hypha/docs") {
			return &annotation
		}
	}

	return nil
}

func GetProvidesAnnotation(res hypha.Resource) *hypha.ResourceAnnotation {
	for _, annotation := range res.Metadata.Annotations {
		if strings.HasPrefix(annotation.Key, "hypha/provides") {
			return &annotation
		}
	}

	return nil
}

func HandleDocs(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	rg := orc.GetResourceGraph()
	if rg.IsEmpty() {
		return nil
	}

	selector := hypha.NewKindResourceSelector("Controller")
	rg.VisitAllMatchingResources(selector, func(idx uint64, res hypha.Resource) bool {
		provides := GetProvidesAnnotation(res)
		if provides != nil {
			if strings.HasPrefix(provides.Value, args[0]) {
				docs := GetDocsAnnotation(res)
				if docs != nil {
					if err := hypha.OpenBrowser(docs.Value); err != nil {
						fmt.Printf("failed to open browser: %v", err)
						return false
					}

					return false
				}
			}
		}

		return true
	})
	return nil
}

func init() {
	docsCmd := &cobra.Command{
		Use:     "docs [kind]",
		Short:   "Open the documentation for a specific resource kind in the system browser",
		Args:    cobra.ExactArgs(1),
		GroupID: "development",
		RunE:    HandleDocs,
	}

	RootCmd.AddCommand(docsCmd)
}
