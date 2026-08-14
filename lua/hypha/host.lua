---@meta
-- host.lua

---@class hypha.Host
local M = {}

---@alias hypha.Host.OS
---| "linux"
---| "darwin"

--- Get the name of the host OS
---@return hypha.Host.OS
function M.getOperatingSystemName() end

--- Get the name of the user
---@return string
function M.getUserName() end

return M
