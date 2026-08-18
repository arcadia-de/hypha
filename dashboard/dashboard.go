package dashboard

import (
	"embed"
)

//go:embed dist/*
var DashboardFS embed.FS
