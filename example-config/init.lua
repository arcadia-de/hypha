local sources = require("hypha.sources")
local runtime = require("hypha.runtime")
runtime.addDefaultLabels({
  "test",
})

local events = require("hypha.events")
events.on("resource.reconciled", function(res)
  -- do work
end)

return {
  sources.file({
    -- "%h/test-symlink.yaml",
    "%h/test-jsonnet.jsonnet",
    "%h/test-test.yaml",
    "%h/test-task.yaml",
  }),
  -- 	sources.raw(
  -- 		[[
  -- kind: Task
  -- metadata:
  --   name: test-task1
  --   labels:
  --      - test
  -- spec:
  --   exec:
  --     timeout: 10
  --     command:
  --     - echo
  --     - \"Hello World\"
  --   check:
  --     timeout: 10
  --     shell: /usr/bin/zsh
  --     command:
  --     - return
  --     - "1"
  --   policy: Always
  -- ]],
  -- 		"yaml"
  -- 	),
  -- 	sources.raw(
  -- 		[[
  -- kind: Directory
  -- metadata:
  --   name: test-dir-1
  --   labels:
  --      - test
  -- spec:
  --   target: "%h/test-dir-1"
  -- ]],
  -- 		"yaml"
  -- 	),
  -- 	sources.raw([[
  -- 	 {
  -- 	   kind: "Directory",
  --      "metadata": {
  --        "name": "TestDirectory",
  --        "labels": [
  --          "test",
  --        ],
  --      },
  -- 	   spec: {
  --        "target": "%h/test-dir",
  --      },
  -- 	 }
  --   ]]),
  -- 	sources.raw([[
  -- 	 {
  -- 	   kind: "Symlink",
  --      "metadata": {
  --        "name": "TestSymlink",
  --        "labels": [
  --          "test",
  --        ],
  --      },
  -- 	   spec: {
  --        "source": "%h/init.lua",
  --        "target": "%h/init-test.lua",
  --      },
  -- 	 }
  -- 	 ]]),
}
