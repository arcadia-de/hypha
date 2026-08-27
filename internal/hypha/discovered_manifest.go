package hypha

/*
#cgo pkg-config: hypha-uninstalled

#include "hypha/discovered_manifest.h"
*/
import "C"

type DiscoveredManifestKind int

const (
	DiscoveredManifestPath       = C.kDiscoveredPath
	DiscoveredManifestRawJson    = C.kDiscoveredRawJson
	DiscoveredManifestRawYaml    = C.kDiscoveredRawYaml
	DiscoveredManifestRawJsonnet = C.kDiscoveredRawJsonnet
)

func (dmk DiscoveredManifestKind) String() string {
	switch dmk {
	case DiscoveredManifestPath:
		return "Path"
	case DiscoveredManifestRawJson:
		return "Raw Json"
	case DiscoveredManifestRawYaml:
		return "Raw Yaml"
	case DiscoveredManifestRawJsonnet:
		return "Raw Jsonnet"
	default:
		return "Unknown"
	}
}

type DiscoveredManifest struct {
	Kind  DiscoveredManifestKind
	Value string
}

type DiscoveredManifestVisitor func(idx uint64, manifest DiscoveredManifest) bool
