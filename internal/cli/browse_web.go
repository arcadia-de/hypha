package cli

import (
	"fmt"
	"time"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/viper"
)

func HandleBrowseWeb() error {
	port := 8080
	address := "0.0.0.0"

	open := viper.GetBool("open")
	if open {
		go func() {
			time.Sleep(500 * time.Millisecond)
			hypha.OpenBrowser(fmt.Sprintf("http://%s:%d", address, port))
		}()
	}

	fmt.Printf("dashboard can be viewed at: http://%s:%d\n", address, port)
	hypha.StartDashboardServer(fmt.Sprintf("%s:%d", address, port))
	return nil
}
