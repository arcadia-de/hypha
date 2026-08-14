---@meta
-- env.lua

---@class hypha.Env
local M = {}

--- Get an environment variable value
---@param name string
---@return string
function M.get(name) end

--- Get all the environment variables
---@return string
function M.all() end

--- Check whether or not an environment variable exists
---@param name string
---@return boolean
function M.has(name) end

return M
