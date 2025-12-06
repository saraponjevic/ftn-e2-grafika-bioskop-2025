#version 330 core

layout(location = 0) in vec2 inPos;  
uniform vec2 uPos;                    
uniform vec2 uScale;                  // skaliranje po x/y

void main()
{
    vec2 worldPos = inPos * uScale + uPos;
    gl_Position = vec4(worldPos, 0.0, 1.0);
}
