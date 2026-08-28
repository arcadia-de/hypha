package hypha

type Node struct {
	Name     string
	Expanded bool
	Children []*Node
}

type Tree struct {
	Nodes []*Node
}

func NewTree(resources []Resource) *Tree {
	tree := &Tree{Nodes: make([]*Node, 0)}
	nodeMap := make(map[string]*Node)
	for _, res := range resources {
		nodeMap[res.Metadata.Name] = &Node{
			Name:     res.Metadata.Name,
			Expanded: true,
			Children: make([]*Node, 0),
		}
	}
	hasParent := make(map[string]bool)

	for _, res := range resources {
		childNode, childExists := nodeMap[res.Metadata.Name]
		if !childExists {
			continue
		}

		for _, parentName := range res.DependsOn {
			if parentNode, parentExists := nodeMap[parentName]; parentExists {
				parentNode.Children = append(parentNode.Children, childNode)
				hasParent[res.Metadata.Name] = true
			}
		}
	}

	for _, res := range resources {
		if !hasParent[res.Metadata.Name] {
			if node, exists := nodeMap[res.Metadata.Name]; exists {
				tree.Nodes = append(tree.Nodes, node)
			}
		}
	}

	return tree
}
