package hypha

type Resource struct {
	ID       string `json:"id,omitempty" yaml:"id,omitempty"`
	Kind     string `json:"kind,omitempty" yaml:"kind,omitempty"`
	State    string `json:"state,omitempty" yaml:"state,omitempty"`
	Action   string
	Reason   string
	Metadata ResourceMetadata `json:"metadata" yaml:"metadata"`
	Spec     string
}
