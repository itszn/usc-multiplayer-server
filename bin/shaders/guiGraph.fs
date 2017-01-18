#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location=1) in vec2 fsTex;
layout(location=0) out vec4 target;

uniform sampler2D graphTex;
uniform vec4 color;

void main()
{
	target = vec4(1.0f);
    
    vec3 col = vec3(0.0f);
    
    float gauge = texture2D(graphTex, vec2(clamp(fsTex.x, 0.01f, 0.99f) , 0.5f)).x;
    
    if (gauge >= 0.75f)
        col = vec3(0.0f, 1.0f, 0.0f);
    else
        col = vec3(1.0f, 0.0f, 0.0f);
    

    target.xyz = col;
    target.a = 1.0f - smoothstep(0.0, 0.02, abs(1.0 - fsTex.y - gauge));
}