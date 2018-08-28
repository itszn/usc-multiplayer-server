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
static int lCreateImage(lua_State* L /*const char* filename, int imageflags */)
{
	const char* filename = luaL_checkstring(L, 1);
	int imageflags = luaL_checkinteger(L, 2);
	int handle = nvgCreateImage(g_vg, filename, imageflags);
	if (handle != 0)
	{
		lua_pushnumber(L, handle);
		return 1;
	}
	return 0;
}
static int lImagePatternFill(lua_State* L /*int image, float alpha*/)
{
	int image = luaL_checkinteger(L, 1);
	float alpha = luaL_checknumber(L, 2);
	int w, h;
	nvgImageSize(g_vg, image, &w, &h);
	nvgFillPaint(g_vg, nvgImagePattern(g_vg, 0, 0, w, h, 0, image, alpha));
	return 0;
}
static int lImageRect(lua_State* L /*float x, float y, float w, float h, int image, float alpha, float angle*/)
{
	float x, y, w, h, alpha, angle;
	int image;
	x = luaL_checknumber(L, 1);
	y = luaL_checknumber(L, 2);
	w = luaL_checknumber(L, 3);
	h = luaL_checknumber(L, 4);
	image = luaL_checkinteger(L, 5);
	alpha = luaL_checknumber(L, 6);
	angle = luaL_checknumber(L, 7);

	int imgH, imgW;
	nvgImageSize(g_vg, image, &imgW, &imgH);
	float scaleX, scaleY;
	scaleX = w / imgW;
	scaleY = h / imgH;
	nvgTranslate(g_vg, x, y);
	nvgRotate(g_vg, angle);
	nvgScale(g_vg, scaleX, scaleY);
	nvgFillPaint(g_vg, nvgImagePattern(g_vg, 0, 0, imgW, imgH, 0, image, alpha));
	nvgRect(g_vg, 0, 0, imgW, imgH);
	nvgFill(g_vg);
	nvgScale(g_vg, 1.0 / scaleX, 1.0 / scaleY);
	nvgRotate(g_vg, -angle);
	nvgTranslate(g_vg, -x, -y);
	return 0;
}
static int lScale(lua_State* L /*float x, float y*/)
{
	float x, y;
	x = luaL_checknumber(L, 1);
	y = luaL_checknumber(L, 2);
	nvgScale(g_vg, x, y);
	return 0;
}
static int lTranslate(lua_State* L /*float x, float y*/)
{
	float x, y;
	x = luaL_checknumber(L, 1);
	y = luaL_checknumber(L, 2);
	nvgTranslate(g_vg, x, y);
	return 0;
}
static int lRotate(lua_State* L /*float angle*/)
{
	float angle = luaL_checknumber(L, 1);
	nvgRotate(g_vg, angle);
	return 0;
}
static int lResetTransform(lua_State* L)
{
	nvgResetTransform(g_vg);
	return 0;
}