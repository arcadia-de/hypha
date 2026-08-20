---@meta
-- The line above tells the LSP this is a definition-only file.

---@module "hypha.packages"
local M = {}

---@alias hypha.packages.PackageStatus
---| "Installed"
---| "Uninstalled"
---| "Error"
---| "Skipped"

---@class hypha.packages.PackageManager
---@field name string The name of the package manager
---@field path string The exec path of the package manager
---@field status fun(pkg: string): hypha.packages.PackageStatus Status a package using the package manager

--- Get a list of package managers
---@return hypha.packages.PackageManager[]
function M.getAllPackageManagers() end

return M
