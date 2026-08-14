---@meta
-- template.lua

---@class hypha.Template
local M = {}

--- Render a template using go-templates
---@param template string The template to render
---@param ctx? string The values to supply the template
---@param yaml? boolean Whether or not the values are yaml. False means they are in json
function M.render(template, ctx, yaml) end

return M
