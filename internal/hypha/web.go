package hypha

import (
	"encoding/json"
	"io/fs"
	"net/http"
	"time"

	dashboard "github.com/arcadia-de/hypha/dashboard"
)

type DataResponse struct {
	Data any `json:"data"`
}

func HandleGetKinds(res http.ResponseWriter, request *http.Request) {
	res.Header().Set("Content-Type", "application/json")
	res.WriteHeader(http.StatusOK)

	data := DataResponse{
		Data: GetAllControllerKinds(),
	}
	if err := json.NewEncoder(res).Encode(data); err != nil {
		http.Error(res, err.Error(), http.StatusInternalServerError)
		return
	}
}

func StartDashboardServer(addr string) error {
	subFS, _ := fs.Sub(dashboard.DashboardFS, "dist")

	mux := http.NewServeMux()
	mux.Handle("/", http.FileServer(http.FS(subFS)))
	mux.HandleFunc("/api/kinds", HandleGetKinds)

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
