---@meta
-- The line above tells the LSP this is a definition-only file.

---@module "hypha.log"
local M = {}

--- Write an info log
---@param msg string The message to log
function M.info(msg) end

--- Write a warn log
---@param msg string The message to log
function M.warn(msg) end

--- Write a success log
---@param msg string The message to log
function M.success(msg) end

--- Write a debug log
---@param msg string The message to log
function M.debug(msg) end

--- Write a error log
---@param msg string The message to log
function M.error(msg) end

--- Write a fatal log
---@param msg string The message to log
function M.fatal(msg) end

return M
