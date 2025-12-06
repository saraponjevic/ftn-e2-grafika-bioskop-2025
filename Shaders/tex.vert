#version 330 core

layout(location = 0) in vec2 inPos;   // lokalna pozicija
layout(location = 1) in vec2 inTex;   // tex koordinate

out vec2 chTex;

uniform vec2 uPos;      // pomeraj u NDC
uniform vec2 uScale;    // skaliranje

void main()
{
    vec2 worldPos = inPos * uScale + uPos;
    gl_Position = vec4(worldPos, 0.0, 1.0);
    chTex = inTex;
}
