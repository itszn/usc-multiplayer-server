#pragma once
#include "nanovg.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

NVGcontext* g_vg;


static int lBeginPath(lua_State* L)
{
	nvgBeginPath(g_vg);
	return 0;
}

static int lText(lua_State* L /*const char* s, float x, float y*/)
{
	const char* s;
	float x, y;
	s = luaL_checkstring(L, 1);
	x = luaL_checknumber(L, 2);
	y = luaL_checknumber(L, 3);
	nvgText(g_vg, x, y, s, 0);
	return 0;
}
static int lFontFace(lua_State* L /*const char* s*/)
{
	const char* s;
	s = luaL_checkstring(L, 1);
	nvgFontFace(g_vg, s);
	return 0;
}
static int lFontSize(lua_State* L /*float size*/)
{
	float size = luaL_checknumber(L, 1);
	nvgFontSize(g_vg, size);
	return 0;
}
static int lFillColor(lua_State* L /*int r, int g, int b*/)
{
	int r, g, b;
	r = luaL_checkinteger(L, 1);
	g = luaL_checkinteger(L, 2);
	b = luaL_checkinteger(L, 3);
	nvgFillColor(g_vg, nvgRGB(r, g, b));
	return 0;
}
static int lRect(lua_State* L /*float x, float y, float w, float h*/)
{
	float x, y, w, h;
	x = luaL_checknumber(L, 1);
	y = luaL_checknumber(L, 2);
	w = luaL_checknumber(L, 3);
	h = luaL_checknumber(L, 4);
	nvgRect(g_vg, x, y, w, h);
	return 0;
}
static int lFill(lua_State* L)
{
	nvgFill(g_vg);
	return 0;
}
static int lTextAlign(lua_State* L /*int align*/)
{
	nvgTextAlign(g_vg, luaL_checkinteger(L, 1));
	return 0;
}