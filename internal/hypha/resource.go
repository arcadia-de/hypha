package hypha

type ResourceStatus struct {
	State     ResourceState    `json:"state" yaml:"state"`
	Action    ControllerAction `json:"action" yaml:"action"`
	Timestamp uint64           `json:"timestamp" yaml:"timestamp"`
	Reason    string           `json:"reason" yaml:"reason"`
}

type Resource struct {
	ID        string           `json:"id,omitempty" yaml:"id,omitempty"`
	Kind      string           `json:"kind,omitempty" yaml:"kind,omitempty"`
	State     string           `json:"state,omitempty" yaml:"state,omitempty"`
	Action    string           `json:"action,omitempty" yaml:"action,omitempty"`
	Reason    string           `json:"reason,omitempty" yaml:"reason,omitempty"`
	Metadata  ResourceMetadata `json:"metadata" yaml:"metadata"`
	DependsOn []string         `json:"depends_on" yaml:"depends_on"`
	Spec      any              `json:"spec" yaml:"spec"`
	Status    ResourceStatus   `json:"status" yaml:"status"`
}

func (res *Resource) HasLabels() bool {
	return len(res.Metadata.Labels) > 0
}

func (res *Resource) HasAnnotations() bool {
	return len(res.Metadata.Annotations) > 0
}
