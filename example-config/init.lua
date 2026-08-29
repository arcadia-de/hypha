local sources = require("hypha.sources")
local runtime = require("hypha.runtime")
runtime.addDefaultLabels({
	"test",
})

return {
	sources.file({
		"%h/test-symlink.yaml",
		"%h/test-test.yaml",
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
	--     - "Hello World"
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
