package hypha

/*
#cgo pkg-config: hypha-uninstalled
#include <stdlib.h>
#include "hypha.h"
#include "hypha/query.h"
#include "hypha/orchestrator.h"
#include "hypha/resource_graph.h"
#include "hypha/resource_query_schema.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

func ParseResultNode(node *C.QueryResult) any {
	if node == nil {
		return nil
	}

	switch node.kind {
	case C.kQueryResultNull:
		return nil

	case C.kQueryResultString:
		return C.GoString(node.string_value)

	case C.kQueryResultArray:
		count := int(node.num_array_items)
		goArray := make([]any, count)

		if count > 0 && node.array_items != nil {
			cItems := unsafe.Slice(node.array_items, count)

			for i := range count {
				goArray[i] = ParseResultNode(cItems[i])
			}
		}
		return goArray

	case C.kQueryResultObject:
		count := int(node.num_object_fields)
		goMap := make(map[string]any, count)

		if count > 0 && node.object_fields != nil {
			cFields := unsafe.Slice(node.object_fields, count)

			for i := range count {
				key := C.GoString(cFields[i].key)
				val := ParseResultNode(cFields[i].value)
				goMap[key] = val
			}
		}

		return goMap

	default:
		return nil
	}
}

// TODO(@s0cks): use this where possible
// goStr := "Hello from Go, no allocations!"
// cPtr := (*C.char)(unsafe.Pointer(unsafe.StringData(goStr)))
// cLen := C.int(len(goStr))
func (orc *Orchestrator) Query(query string) (any, error) {
	cResourceGraph := C.GetOrcResourceGraph(orc.Handle)
	if cResourceGraph == nil {
		return nil, fmt.Errorf("orchestrator has no resource graph")
	}

	cRqctx := (*C.ResourcesQueryContext)(C.malloc(C.sizeof_ResourcesQueryContext))
	defer C.free(unsafe.Pointer(cRqctx))
	cRqctx.resources = C.GetResourceGraphResources(cResourceGraph)
	cRqctx.count = C.GetNumberOfResourcesInResourceGraph(cResourceGraph)

	cQuery := C.CString(query)
	cQuerySchema := C.HyphaResourcesQuerySchema(cRqctx)

	var cErr *C.char
	cQueryResult := C.QueryExecute(&cQuerySchema, cQuery, &cErr)
	if cQueryResult == nil {
		err_message := C.GoString(cErr)
		C.free(unsafe.Pointer(cErr))
		return nil, fmt.Errorf("failed to execute query: %s", err_message)
	}

	data := C.ResultNodeToJSON(cQueryResult)
	C.ResultNodeFree(cQueryResult)
	res := C.GoString(data)
	defer C.free(unsafe.Pointer(data))

	// var res any
	// if err := json.Unmarshal([]byte(C.GoString(data)), &res); err != nil {
	// 	return nil, fmt.Errorf("failed to unmarshal data: %v", err)
	// }

	return res, nil
}
