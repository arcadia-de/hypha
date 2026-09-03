package cli

type ValidateSummary struct {
	Valid   int
	Invalid int
	Total   int
}

func (summary *ValidateSummary) HasInvalid() bool {
	return summary.Invalid > 0
}

func (summary *ValidateSummary) HasValid() bool {
	return summary.Valid > 0
}

func (summary *ValidateSummary) IsInvalid() bool {
	return summary.Invalid == summary.Total
}

func (summary *ValidateSummary) IsValid() bool {
	return summary.Valid == summary.Total
}
