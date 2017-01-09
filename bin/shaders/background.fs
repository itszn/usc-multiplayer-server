#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location=1) in vec2 texVp;
layout(location=0) out vec4 target;

uniform ivec2 screenCenter;
// x = bar time
// y = object glow
uniform vec2 timing;
uniform ivec2 viewport;
uniform float objectGlow;
uniform sampler2D mainTex;
uniform float tilt;

#define pi 3.1415926535897932384626433832795

vec2 rotate_point(vec2 cen,float angle,vec2 p)
{
  float s = sin(angle);
  float c = cos(angle);

  // translate point back to origin:
  p.x -= cen.x;
  p.y -= cen.y;

  // rotate point
  float xnew = p.x * c - p.y * s;
  float ynew = p.x * s + p.y * c;

  // translate point back:
  p.x = xnew + cen.x;
  p.y = ynew + cen.y;
  return p;
}


void main()
{
    float ar = viewport.x / viewport.y;
    vec2 center = vec2(screenCenter);
	vec2 uv = texVp.xy;
	uv.x *= ar;

    center.x *= ar;
    uv = rotate_point(center, tilt * 2.0 * pi, uv);
    float thing = 1.8 / abs(center.x - uv.x);
    float thing2 = abs(center.x - uv.x) * 2.0;
    uv.y -= center.y;
    uv.y *=  thing;
    uv.y = (uv.y + 1.0) / 2.0;
    uv.x *= thing / 3.0;
    uv.x += timing.x * 10.0;
	
    vec4 col = texture2D(mainTex, uv);
    if (abs(uv.y) > 1.0 || uv.y < 0.0)
        col = vec4(0);
    col.a *= 1.0 - (thing * 70.0);
    col *= vec4(col.a);

	target = col;
    
}