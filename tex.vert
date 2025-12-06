#version 330 core

layout(location = 0) in vec2 inPos;  
layout(location = 1) in vec2 inTex;   

out vec2 chTex;

uniform vec2 uPos;      
uniform vec2 uScale;   

void main()
{
    vec2 worldPos = inPos * uScale + uPos;
    gl_Position = vec4(worldPos, 0.0, 1.0);
    chTex = inTex;
}
