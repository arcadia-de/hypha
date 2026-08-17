package cli

type Symbol struct {
	Color string
	NF    string
	ASCII string
}

var (
	CreateSymbol = Symbol{
		Color: "#66800B",
		NF:    "󰐙",
		ASCII: "+",
	}

	UpdateSymbol = Symbol{
		Color: "#5E409D",
		NF:    "󰚰",
		ASCII: "~",
	}

	DestroySymbol = Symbol{
		Color: "#AF3029",
		NF:    "󰩺",
		ASCII: "-",
	}

	NoOpSymbol = Symbol{
		Color: "#403E3C",
		NF:    "󰄭",
		ASCII: " ",
	}
)
