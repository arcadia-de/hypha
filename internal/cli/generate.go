package cli

import (
	"fmt"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
	"strings"
)

var manifestLabels []string
var manifestAnnotations map[string]string

func getLabelsAndAnnotations() ([]string, []hypha.ResourceAnnotation, error) {
	labels := viper.GetStringSlice("labels")

	var annotations []hypha.ResourceAnnotation
	cmdAnnotations := viper.GetStringMapString("annotations")
	for k, v := range cmdAnnotations {
		annotations = append(annotations, hypha.ResourceAnnotation{
			Key:   k,
			Value: v,
		})
	}

	return labels, annotations, nil
}

func handleGenerate(kind string, args []string) error {
	id := viper.GetString("id")
	if id == "" {
		//TODO(@s0cks): generate a new ID
	}

	format := viper.GetString("format")
	name := viper.GetString("name")

	labels, annotations, err := getLabelsAndAnnotations()
	if err != nil {
		return err
	}

	spec := hypha.ResourceSpec{
		Kind: hypha.Capitalize(kind),
		ID:   id,
		Metadata: hypha.ResourceMetadata{
			Name:        name,
			Labels:      labels,
			Annotations: annotations,
		},
	}

	filename := viper.GetString("output")
	if filename == "" {
		filename = fmt.Sprintf("%s-%s", kind, id)
	}

	if !strings.HasSuffix(filename, "."+format) {
		filename = fmt.Sprintf("%s.%s", filename, format)
	}

	err = hypha.WriteManifestToFile(strings.ToLower(filename), format, spec)
	if err != nil {
		return fmt.Errorf("failed to write new manifest file %s: %v", filename, err)
	}

	return nil
}

func createGenResourceCommand(kind string) *cobra.Command {
	actionCmd := &cobra.Command{
		Use: kind + "s",
		Aliases: []string{
			kind,
		},
		Short: fmt.Sprintf("Generate a %s manifest", hypha.Capitalize(kind)),
		RunE: func(cmd *cobra.Command, args []string) error {
			return handleGenerate(kind, args)
		},
	}

	actionCmd.Flags().StringP("output", "o", "", "The name of the output file")
	actionCmd.Flags().StringP("format", "f", "yaml", "The format for the generated file. Accepts: [yaml|json]")
	actionCmd.Flags().StringP("name", "n", "", "The name of the resource")
	actionCmd.Flags().StringSliceVarP(
		&manifestLabels,
		"labels",
		"l",
		[]string{},
		"A list of labels to add to the new manifest. Accepts: value",
	)
	actionCmd.Flags().StringToStringVarP(
		&manifestAnnotations,
		"annotations",
		"a",
		map[string]string{},
		"Add annotations to the new manifest. Accepts: key=value",
	)

	return actionCmd
}

func CreateGenCommand(kinds []string) *cobra.Command {
	generateCmd := &cobra.Command{
		Use:   "generate",
		Short: "generate a manifest for a given resource",
		Aliases: []string{
			"gen",
		},
		GroupID: "config",
	}

	for _, kind := range kinds {
		generateCmd.AddCommand(createGenResourceCommand(kind))
	}

	return generateCmd
}
