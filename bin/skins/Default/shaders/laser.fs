#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location=1) in vec2 fsTex;
layout(location=0) out vec4 target;

uniform sampler2D mainTex;
uniform vec4 color;
uniform float objectGlow;

// 20Hz flickering. 0 = Miss, 1 = Inactive, 2 & 3 = Active alternating.
uniform int hitState;

// 0 = body, 1 = entry, 2 = exit
uniform int laserPart;

void main()
{	
    float x = fsTex.x;
    float laserSize = 1.0; //0.0 to 1.0
    x -= 0.5;
    x /= laserSize;
    x += 0.5;
	vec4 mainColor = texture(mainTex, vec2(x,fsTex.y));
	target = mainColor * color;
	float brightness = (target.x + target.y + target.z) / 3;
	target.xyz = target.xyz * (0 + objectGlow * 1.2);
}