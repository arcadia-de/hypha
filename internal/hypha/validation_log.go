package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "hypha/validation_log.h"

bool goVisitValidationResult(const size_t, ValidationResult* result, void* data);
*/
import "C"

import (
	"fmt"
	"runtime"
	"runtime/cgo"
	"strings"
	"unsafe"

	lg "charm.land/lipgloss/v2"
	"github.com/spf13/viper"
)

type ValidationResultKind int

const (
	SkippedValidationResult = C.kValidationSkipped
	PassedValidationResult  = C.kValidationPassed
	WarningValidationResult = C.kValidationWarning
	FailedValidationResult  = C.kValidationFailed
)

type ValidationResult struct {
	Name   string
	Kind   ValidationResultKind
	Reason string
}

type ValidationLog struct {
	Results []ValidationResult
	Passed  int
	Failed  int
	Skipped int
	Warning int
	Unknown int
}

func (vlog *ValidationLog) IsValid() bool {
	return vlog.Passed == len(vlog.Results)
}

func (vlog *ValidationLog) IsSkipped() bool {
	return vlog.Skipped == len(vlog.Results)
}

type ValidationResultVisitor func(idx uint64, result ValidationResult) bool

//export goVisitValidationResult
func goVisitValidationResult(idx C.uint64_t, rec *C.ValidationResult, data unsafe.Pointer) C.bool {
	handle := *(*cgo.Handle)(data)
	vis := handle.Value().(ValidationResultVisitor)

	goName := C.GoString(rec.resource.info.name)

	rawReason := C.GoStringN(&rec.reason[0], C.int(C.HYPHA_REASON_MAX_LENGTH))
	goReason, _, _ := strings.Cut(rawReason, "\x00")

	goKind := ValidationResultKind(rec.kind)
	goResult := ValidationResult{
		Name:   goName,
		Kind:   goKind,
		Reason: goReason,
	}
	return C.bool(vis(uint64(idx), goResult))
}

func VisitAllValidationResults(vl *C.ValidationLog, vis ValidationResultVisitor) {
	handle := cgo.NewHandle(vis)
	defer handle.Delete()

	C.VisitAllValidationResults(
		vl,
		(C.VisitValidationLogFn)(unsafe.Pointer(C.goVisitValidationResult)),
		unsafe.Pointer(&handle),
	)

	runtime.KeepAlive(handle)
}

func GetValidationLog(vl *C.ValidationLog) ValidationLog {
	var (
		num_pass    = 0
		num_failed  = 0
		num_skipped = 0
		num_warning = 0
		num_unknown = 0
	)

	var results []ValidationResult
	VisitAllValidationResults(vl, func(idx uint64, result ValidationResult) bool {
		switch result.Kind {
		case SkippedValidationResult:
			num_skipped++
		case PassedValidationResult:
			num_pass++
		case FailedValidationResult:
			num_failed++
		case WarningValidationResult:
			num_warning++
		default:
			num_unknown++
		}

		results = append(results, result)
		return true
	})

	return ValidationLog{
		Results: results,
		Passed:  num_pass,
		Failed:  num_failed,
		Skipped: num_skipped,
		Warning: num_warning,
		Unknown: num_unknown,
	}
}

func (vlog *ValidationLog) IsEmpty() bool {
	return len(vlog.Results) == 0
}

func GetValidationResultKindName(kind ValidationResultKind) string {
	switch kind {
	case SkippedValidationResult:
		return "Skipped"
	case PassedValidationResult:
		return "Passed"
	case FailedValidationResult:
		return "Failed"
	case WarningValidationResult:
		return "Warning"
	default:
		return "Unknown"
	}
}

func GetValidationResultKindIcon(kind ValidationResultKind) string {
	switch kind {
	case SkippedValidationResult:
		return ""
	case PassedValidationResult:
		return ""
	case FailedValidationResult:
		return ""
	case WarningValidationResult:
		return ""
	default:
		return ""
	}
}

func (vlog *ValidationLog) Print(cs ColorScheme) {
	verbose := viper.GetBool("verbose")
	if vlog.IsValid() || vlog.IsSkipped() {
		if !verbose {
			return
		}
	}

	rowStyle := lg.NewStyle().
		MarginLeft(2)
	style := NewValidationLogStyle(&rowStyle, &cs)
	fmt.Println(style.HeaderRowStyle.Render(
		lg.JoinHorizontal(
			lg.Left,
			style.HeaderKindStyle.Render("Status"),
			style.HeaderKindStyle.Render("Name"),
			style.HeaderKindStyle.Render("Reason"),
		),
	))

	for _, result := range vlog.Results {
		style.PrintRow(&result, verbose)
	}
	fmt.Println()

	skippedRowStyle := rowStyle.
		Foreground(cs.GetSkippedColor())
	passedRowStyle := rowStyle.
		Foreground(cs.GetPassedColor())
	failedRowStyle := rowStyle.
		Foreground(cs.GetFailedColor())
	warningRowStyle := rowStyle.
		Foreground(cs.GetWarningColor())
	unknownRowStyle := rowStyle.
		Foreground(cs.GetUnknownColor())
	if vlog.Passed > 0 && verbose {
		fmt.Println(skippedRowStyle.Render(
			fmt.Sprintf("%s %d/%d %s", GetValidationResultKindIcon(SkippedValidationResult), vlog.Skipped, len(vlog.Results), GetValidationResultKindName(SkippedValidationResult)),
		))
	}

	if vlog.Skipped > 0 && verbose {
		fmt.Println(passedRowStyle.Render(
			fmt.Sprintf("%s %d/%d %s", GetValidationResultKindIcon(PassedValidationResult), vlog.Passed, len(vlog.Results), GetValidationResultKindName(PassedValidationResult)),
		))
	}

	if vlog.Warning > 0 {
		fmt.Println(warningRowStyle.Render(
			fmt.Sprintf("%s %d/%d %s", GetValidationResultKindIcon(WarningValidationResult), vlog.Warning, len(vlog.Results), GetValidationResultKindName(WarningValidationResult)),
		))
	}

	if vlog.Failed > 0 {
		fmt.Println(failedRowStyle.Render(
			fmt.Sprintf("%s %d/%d %s", GetValidationResultKindIcon(FailedValidationResult), vlog.Failed, len(vlog.Results), GetValidationResultKindName(FailedValidationResult)),
		))
	}

	if vlog.Unknown > 0 {
		fmt.Println(unknownRowStyle.Render(
			fmt.Sprintf("%s %d/%d %s", GetValidationResultKindIcon(-1), vlog.Unknown, len(vlog.Results), GetValidationResultKindName(-1)),
		))
	}
}
