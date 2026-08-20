---@meta
-- The line above tells the LSP this is a definition-only file.

---@module "hypha.sources"
local M = {}

--- Get manifests by a file path
---@param files string|string[] The file or files to resolve
---@return string[] - The resolved manifest files
function M.file(files) end

---@class hypha.source.DirOpts
---@field recursive? boolean Whether or not to be recursive

--- Get manifests by a dir
---@param dir string The directory to resolve manifests from
---@param opts? hypha.source.DirOpts The options to use
---@return string[] - The resolved manifest files
function M.dir(dir, opts) end

---@class hypha.source.GlobOpts
---@field recursive? boolean Whether or not to be recursive

--- Get manifests
---@param pattern string The pattern to resolve manifests from
---@param opts? hypha.source.GlobOpts The options to use
---@return string[] - The resolved manifest files
function M.glob(pattern, opts) end

--- Get manifests
---@param text string The raw data
---@return string[] - The resolved manifest files
function M.raw(text) end

return M
