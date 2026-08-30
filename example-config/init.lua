local sources = require("hypha.sources")
local runtime = require("hypha.runtime")
runtime.addDefaultLabels({
	"test",
})

return {
	sources.file("%h/test-test.yaml"),
}
