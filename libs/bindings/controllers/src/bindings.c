LUA_FN(createController) {
  const char* kind = luaL_checkstring(L, 1);
  Controller* ctrl = NewLuaController(L, kind, 2);
  if (!ctrl)
    return luaL_error(L, "failed to create new lua controller for kind: %s", kind);
  ControllerInit(ctrl);
  return 0;
}
