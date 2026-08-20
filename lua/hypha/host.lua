---@meta
-- host.lua

---@module "hypha.host"
local M = {}

---@class hypha.host.KernelInfo
---@field version string
---@field release string
---@field sysname string
---@field machine string
---@field nodename string

---@class hypha.host.HostInfo
---@field os string
---@field arch string
---@field hostname string
---@field username string

---@alias hypha.host.OS
---| "linux"
---| "darwin"
---| "windows"

--- Get all the info about the host
---@return hypha.host.HostInfo
function M.info() end

--- Get the name of the host OS
---@return hypha.host.OS
function M.os() end

--- Get the name of the user
---@return string
function M.username() end

--- Get the hostname
---@return string
function M.hostname() end

--- Get the kernel info
---@return hypha.host.KernelInfo
function M.kernel() end

--- Get the arch
---@return string
function M.arch() end

--- Get the OS distribution
---@return string
function M.distro() end

--- Check whether or not the system has a package
---@param pkg string
---@return boolean
function M.has(pkg) end

--- Execute /usr/bin/which for a specific bin
---@param bin string The binary to search for
---@return string
function M.find(bin) end

return M
