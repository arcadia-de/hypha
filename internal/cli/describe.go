package cli

import (
	"fmt"
	"strings"

	lg "charm.land/lipgloss/v2"
	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func CreateDescribeFilter(kind string, refs []string) hypha.ResourceSelector {
	kindFilter := hypha.NewKindResourceSelector(kind)
	refsFilter := hypha.NewRefsResourceSelector(refs)
	return hypha.NewAndResourceSelector([]hypha.ResourceSelector{
		kindFilter,
		refsFilter,
	})
}

var (
	SummaryTitleStyle = lg.NewStyle()

	SummaryFieldStyle = lg.NewStyle().
				Width(15).
				PaddingLeft(2)

	SummaryValueStyle = lg.NewStyle()

	SummaryLabelStyle = lg.NewStyle().
				PaddingLeft(4)

	SummaryAnnotationStyle = lg.NewStyle().
				PaddingLeft(4)
)

func PrintResourceSummaryField(name string, value string) {
	fmt.Printf("%s %s\n", SummaryFieldStyle.Render(fmt.Sprintf("%s:", name)), SummaryValueStyle.Render(value))
}

func PrintResourceAnnotations(annotations []hypha.ResourceAnnotation) {
	fmt.Printf("%s\n", SummaryFieldStyle.Render("annotations:"))
	for _, annotation := range annotations {
		annot := fmt.Sprintf("%s=%s", annotation.Key, annotation.Value)
		fmt.Printf("%s\n", SummaryAnnotationStyle.Render(annot))
	}
}

func PrintResourceLabels(labels []string) {
	fmt.Printf("%s\n", SummaryFieldStyle.Render("labels:"))
	for _, label := range labels {
		fmt.Printf("%s\n", SummaryLabelStyle.Render(label))
	}
}

func PrintResourceSummary(resource hypha.Resource) {
	// TestRawManifest2
	//	kind:        test
	//	id:          0
	//	labels:      env=dev, owner=tazz
	//	annotations: source-kind=raw, hypha.io/managed-by=controller-x
	//	spec:
	//	  <jsonnet-resolved fields>
	//	state:
	//	  last action:  Create
	//	  last status:  ok
	//	  last applied: 2026-08-14T09:12:03Z
	fmt.Printf("%s\n", SummaryTitleStyle.Render(resource.ID))
	PrintResourceSummaryField("kind", resource.Kind)
	PrintResourceSummaryField("id", resource.ID)

	if len(resource.Metadata.Labels) > 0 {
		PrintResourceLabels(resource.Metadata.Labels)
	}

	if len(resource.Metadata.Annotations) > 0 {
		PrintResourceAnnotations(resource.Metadata.Annotations)
	}

	if len(resource.Spec) > 0 {
		fmt.Printf("%s\n", SummaryFieldStyle.Render(fmt.Sprintf("spec: %s", resource.Spec)))
	}

	// TODO(@s0cks): fmt.Printf("%s\n", SummaryFieldStyle.Render("state:"))
}

func handleDescribe(kind string, args []string) error {
	_ = kind

	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create Orchestrator: %v", err)
	}
	defer orc.Close()

	orc.ProcessDiscoveredManifests()

	fmt.Println()

	filter := CreateDescribeFilter(kind, args)
	defer filter.Close()

	rg := orc.GetResourceGraph()
	resources := rg.ListResourcesWithSelector(filter)
	for _, r := range resources {
		fmt.Println()
		PrintResourceSummary(r)
	}
	fmt.Println()

	return nil
}

func createDescribeResourceCommand(kind string) *cobra.Command {
	return &cobra.Command{
		Use: kind + "s",
		Aliases: []string{
			kind,
			strings.ToLower(kind),
			strings.ToLower(kind) + "s",
		},
		Short: fmt.Sprintf("Describe %s resources", kind),
		Args:  cobra.MinimumNArgs(1),
		RunE: func(cmd *cobra.Command, args []string) error {
			return handleDescribe(kind, args)
		},
	}
}

func CreateDescribeCommand(kinds []string) *cobra.Command {
	describeCmd := &cobra.Command{
		Use: "describe",
		Aliases: []string{
			"desc",
		},
		Short:   "Describe a resource",
		GroupID: "inspection",
	}

	for _, kind := range kinds {
		describeCmd.AddCommand(createDescribeResourceCommand(kind))
	}

	return describeCmd
}
