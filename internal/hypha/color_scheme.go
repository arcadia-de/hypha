package hypha

import (
	lg "charm.land/lipgloss/v2"

	"image/color"
)

type ColorScheme struct {
	ProcessingColor string
	ReadyColor      string
	FailedColor     string
	ErrorColor      string
	UnknownColor    string
	NoneColor       string
	CreateColor     string
	UpdateColor     string
	DestroyColor    string
	SkippedColor    string
	PassedColor     string
	PendingColor    string
	WarningColor    string
	MutedColor      string
	BorderColor     string
}

func GetDefaultColorScheme() ColorScheme {
	return ColorScheme{
		ReadyColor:   "#66800B",
		FailedColor:  "#AF3029",
		ErrorColor:   "#AF3029",
		UnknownColor: "#403E3C",

		CreateColor:  "#66800B",
		UpdateColor:  "#5E409D",
		DestroyColor: "#AF3029",
		NoneColor:    "#403E3C",

		PassedColor:     "#66800B",
		ProcessingColor: "#3AA99F",
		SkippedColor:    "#D0A215",
		WarningColor:    "#DA702C",
		PendingColor:    "#D0A215",

		BorderColor: "#282726",
		MutedColor:  "#878580",
	}
}

func (cs *ColorScheme) GetPendingColor() color.Color {
	return lg.Color(cs.PendingColor)
}

func (cs *ColorScheme) GetWarningColor() color.Color {
	return lg.Color(cs.WarningColor)
}

func (cs *ColorScheme) GetPassedColor() color.Color {
	return lg.Color(cs.PassedColor)
}

func (cs *ColorScheme) GetSkippedColor() color.Color {
	return lg.Color(cs.SkippedColor)
}

func (cs *ColorScheme) GetBorderColor() color.Color {
	return lg.Color(cs.BorderColor)
}

func (cs *ColorScheme) GetMutedColor() color.Color {
	return lg.Color(cs.MutedColor)
}

func (cs *ColorScheme) GetProcessingColor() color.Color {
	return lg.Color(cs.ProcessingColor)
}

func (cs *ColorScheme) GetReadyColor() color.Color {
	return lg.Color(cs.ReadyColor)
}

func (cs *ColorScheme) GetFailedColor() color.Color {
	return lg.Color(cs.FailedColor)
}

func (cs *ColorScheme) GetErrorColor() color.Color {
	return lg.Color(cs.ErrorColor)
}

func (cs *ColorScheme) GetUnknownColor() color.Color {
	return lg.Color(cs.UnknownColor)
}

func (cs *ColorScheme) GetNoneColor() color.Color {
	return lg.Color(cs.NoneColor)
}

func (cs *ColorScheme) GetCreateColor() color.Color {
	return lg.Color(cs.CreateColor)
}

func (cs *ColorScheme) GetUpdateColor() color.Color {
	return lg.Color(cs.UpdateColor)
}

func (cs *ColorScheme) GetDestroyColor() color.Color {
	return lg.Color(cs.DestroyColor)
}
