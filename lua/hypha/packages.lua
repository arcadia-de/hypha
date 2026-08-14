---@meta
-- The line above tells the LSP this is a definition-only file.

---@module "hypha.packages"
local M = {}

---@alias hypha.PackageStatus
---| "Installed"
---| "Uninstalled"
---| "Error"
---| "Skipped"

---@alias hypha.PackageManager.StatusFunction fun(pkg: string): hypha.PackageStatus
---@alias hypha.PackageManager.InstallFunction fun(pkg: string): hypha.PackageStatus
---@alias hypha.PackageManager.UninstallFunction fun(pkg: string): hypha.PackageStatus

---@class hypha.PackageManager
---@field name string The name of the package manager
---@field path string The exec path of the package manager
---@field status hypha.PackageManager.StatusFunction Status a package using the package manager
---@field install hypha.PackageManager.InstallFunction Install a package using the package manager
---@field uninstall hypha.PackageManager.UninstallFunction Uninstall a package using the package manager

---@alias hypha.PackageManagerList table<hypha.PackageManager>

--- Get a list of package managers
---@return hypha.PackageManagerList
function M.getAllPackageManagers() end

return M
