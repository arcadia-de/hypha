#ifdef HYPHA_WAMR_ENABLED

#ifndef HYPHA_WASM_CONTROLLER_H
#define HYPHA_WASM_CONTROLLER_H

#ifndef HYPHA_WASM_ERRBUF_SIZE
#define HYPHA_WASM_ERRBUF_SIZE 128
#endif  // HYPHA_WASM_ERRBUF_SIZE

#ifndef HYPHA_WASM_RUNTIME_SIZE
#define HYPHA_WASM_RUNTIME_SIZE 8192
#endif  // HYPHA_WASM_RUNTIME_SIZE

#include <wasm_export.h>

struct _WasmController {
  wasm_module_t wasm_module;
  wasm_module_inst_t wasm_module_inst;
  wasm_exec_env_t wasm_exec_env;

  wasm_function_inst_t observe;
  wasm_function_inst_t plan;
  wasm_function_inst_t apply;

  void* data;
};

#endif  // HYPHA_WASM_CONTROLLER_H

#endif  // HYPHA_WAMR_ENABLED
