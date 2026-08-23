package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>

#include "hypha.h"
*/
import "C"

import (
	md "github.com/lrstanley/go-nf/glyphs/md"
)

type ControllerStatus int

const (
	kOkStatus             ControllerStatus = C.kStatusOk
	kNoOpStatus           ControllerStatus = C.kStatusNoOp
	kInvalidSpecStatus    ControllerStatus = C.kStatusInvalidSpec
	kNotFoundStatus       ControllerStatus = C.kStatusNotFound
	kConflictStatus       ControllerStatus = C.kStatusConflict
	kUnsupportedStatus    ControllerStatus = C.kStatusUnsupported
	kTransientErrorStatus ControllerStatus = C.kStatusTransientError
	kPermanentErrorStatus ControllerStatus = C.kStatusPermanentError
	kInternalErrorStatus  ControllerStatus = C.kStatusInternalError
)

func (status ControllerStatus) GetName() string {
	switch status {
	case kOkStatus:
		return "Ok"
	case kNoOpStatus:
		return "NoOp"
	case kInvalidSpecStatus:
		return "Invalid Spec"
	case kNotFoundStatus:
		return "Not Found"
	case kConflictStatus:
		return "Conflict"
	case kUnsupportedStatus:
		return "Unsupported"
	case kTransientErrorStatus:
		return "Transient Error"
	case kPermanentErrorStatus:
		return "Permanent Error"
	case kInternalErrorStatus:
		return "Internal Error"
	default:
		return "Unknown"
	}
}

func (status ControllerStatus) GetIcon() string {
	switch status {
	case kOkStatus:
		return md.Check.String()
	case kNoOpStatus:
		return md.Minus.String()
	case kInvalidSpecStatus:
		return md.AlertCircle.String()
	case kNotFoundStatus:
		return md.HelpCircle.String()
	case kConflictStatus:
		return md.Compare.String()
	case kUnsupportedStatus:
		return md.Cancel.String()
	case kTransientErrorStatus:
		return md.Refresh.String()
	case kPermanentErrorStatus:
		return md.Alert.String()
	case kInternalErrorStatus:
		return md.AlertOctagon.String()
	default:
		return md.CommentQuestion.String()
	}
}
