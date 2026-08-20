---@meta
-- init.lua

---@module "hypha.Hypha"
local M = {}

--- Get the current version of Hypha
---@return string
function M.version() end

--- Expand a string
---@param value string
---@return string
function M.expand(value) end

--- Get the current working directory
---@return string
function M.cwd() end

--- Render a template using go-templates
---@param template string The template to render
---@param ctx? string The values to supply the template in yaml or json
---@param yaml? boolean Whether or not the values are yaml. False means they are in json
function M.renderTemplate(template, ctx, yaml) end

--- Render a Jsonnet file using go-jsonnet
---@param filename string The name of the Jsonnet file
---@param content string The contents of the Jsonnet file
function M.renderJsonnet(filename, content) end

return M
