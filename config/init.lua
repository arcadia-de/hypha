local log = require("hypha.log")
-- local resource = require("hypha.resource")
--
-- print("hypha version: v" .. hypha.getVersion())
--
-- local data, err = resource.query([[
--   resources(kind: Package){ id }
-- ]])
--
-- if err then
-- 	log.error("error querying resources: " .. err)
-- 	return
-- end
--
-- log.success("data: " .. inspect(data))

log.info("Hello World")
local template = require("hypha.template")

local tpl = [[
echo "{{ .message }}"
]]

local data = [[
{
  "message": "Hello World"
}
]]

local result = template.render(tpl, data)

log.info("Test")
log.info("result: " .. result)

log.info("rendering jsonnet")
local jsonnet = require("hypha.jsonnet")
local rendered = jsonnet.render(
	"test.jsonnet",
	[[
  local message = "Hello World";
  {
    message: message,
  }
  ]]
)
log.info("result: " .. rendered)
