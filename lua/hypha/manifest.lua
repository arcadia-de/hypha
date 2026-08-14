---@meta
-- The line above tells the LSP this is a definition-only file.

---@class hypha.Manifest
local M = {}

--- Get manifests by a file path
---@param files string|table<string> The file or files to resolve
---@return table<string> - The resolved manifest files
function M.file(files) end

---@class hypha.Manifest.DirOpts
---@field recursive boolean Whether or not to be recursive

--- Get manifests by a dir
---@param dir string The directory to resolve manifests from
---@param opts? hypha.Manifest.DirOpts The options to use
---@return table<string> - The resolved manifest files
function M.dir(dir, opts) end

---@class hypha.Manifest.GlobOpts
---@field recursive boolean Whether or not to be recursive

--- Get manifests
---@param pattern string The pattern to resolve manifests from
---@param opts? hypha.Manifest.GlobOpts The options to use
---@return table<string> - The resolved manifest files
function M.glob(pattern, opts) end

--- Get manifests
---@param text string The raw data
---@return table<string> - The resolved manifest files
function M.raw(text) end

return M
