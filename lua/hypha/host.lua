---@meta
-- host.lua

---@class hypha.Host
local M = {}

---@alias hypha.Host.OS
---| "linux"
---| "darwin"

--- Get the name of the host OS
---@return hypha.Host.OS
function M.getOS() end

--- Get the name of the user
---@return string
function M.getUsername() end

--- Get the hostname
---@return string
function M.getHostname() end

---@class hypha.Host.KernelInfo
---@field version string
---@field release string
---@field sysname string
---@field machine string
---@field nodename string

--- Get the kernel info
---@return string
function M.getKernelInfo() end

--- Get the kernel version
---@return string
function M.getKernelVersion() end

--- Get the arch
---@return string
function M.getArch() end

--- Get the OS distribution
---@return string
function M.getDistro() end

--- Check whether or not the system is OSX
---@return boolean
function M.isOSX() end

--- Check whether or not the system is Linux
---@return boolean
function M.isLinux() end

--- Check whether or not the system is Unix
---@return boolean
function M.isUnix() end

--- Check whether or not the system is Windows
---@return boolean
function M.isWindows() end

--- Check whether or not the system is FreeBSD
---@return boolean
function M.isFreeBSD() end

--- Check whether or not the system has a package
---@param pkg string
---@return boolean
function M.has(pkg) end

return M
