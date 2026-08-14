package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include <stdbool.h>
#include "hypha.h"
*/
import "C"
import (
	"bytes"
	"encoding/json"
	"gopkg.in/yaml.v3"
	"text/template"
)

//export RenderTemplate
func RenderTemplate(tpl *C.char, data *C.char, isYaml C.bool) *C.char {
	goTmpl := C.GoString(tpl)
	goData := C.GoString(data)

	var tplData any
	var err error
	if isYaml {
		err = yaml.Unmarshal([]byte(goData), &tplData)
	} else {
		err = json.Unmarshal([]byte(goData), &tplData)
	}

	if err != nil {
		return C.CString("Error parsing data: " + err.Error())
	}

	tmpl, err := template.New("cgo").Parse(goTmpl)
	if err != nil {
		return C.CString("Error parsing template: " + err.Error())
	}

	var buf bytes.Buffer
	if err := tmpl.Execute(&buf, tplData); err != nil {
		return C.CString("Error executing template: " + err.Error())
	}

	return C.CString(buf.String())
}

//export RenderJsonnet
func RenderJsonnet(name *C.char, code *C.char) *C.char {
	vm := CreateJsonnetVM()
	goCode := C.GoString(code)
	goName := C.GoString(name)

	rendered, err := RenderJsonnetManifest(vm, goName, goCode)
	if err != nil {
		return C.CString("error rendering jsonnet: " + err.Error())
	}

	return C.CString(rendered)
}
