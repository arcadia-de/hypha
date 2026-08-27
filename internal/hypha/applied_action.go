package hypha

/*
#cgo pkg-config: hypha-uninstalled
*/
import "C"

type AppliedAction struct {
	Action uint32
	Name   string
	Kind   string
	Reason string
}

type AppliedActionVisitor func(idx uint64, act AppliedAction) bool
