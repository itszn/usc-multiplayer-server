#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location=1) in vec2 fsTex;
layout(location=0) out vec4 target;

uniform float time;
uniform float rate;
uniform sampler2D mainTex;
uniform sampler2D maskTex;
uniform vec4 barColor;

void main()
{
	vec4 tex = texture2D(mainTex, fsTex);
    tex.xyz *= 0.5;
    if(rate >= 0.7)
        tex.xyz *= 2;
    float mask = texture2D(maskTex, fsTex).x;
    mask -= 1.0 - rate;
    mask *= 100;
    mask = clamp(mask, 0.0, 1.0);
	target.rgb = tex.rgb;
    target.a = tex.a * mask;
}