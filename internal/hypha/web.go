package hypha

import (
	"io/fs"
	"net/http"
	"time"

	dashboard "github.com/arcadia-de/hypha/dashboard"
)

func StartDashboardServer(addr string) error {
	subFS, _ := fs.Sub(dashboard.DashboardFS, "dist")

	mux := http.NewServeMux()
	mux.Handle("/", http.FileServer(http.FS(subFS)))
	// mux.HandleFunc("/", func(res http.ResponseWriter, request *http.Request) {
	// 	res.Header().Set("Content-Type", "text/plain")
	// })

	server := &http.Server{
		Addr:         addr,
		Handler:      mux,
		ReadTimeout:  5 * time.Second,
		WriteTimeout: 5 * time.Second,
	}

	return server.ListenAndServe()
}

func OpenBrowser(url string) error {
	cmd := CreateOpenCommand(url)
	if cmd != nil {
		err := cmd.Start()
		if err != nil {
			return err
		}
	}

	return nil
}
