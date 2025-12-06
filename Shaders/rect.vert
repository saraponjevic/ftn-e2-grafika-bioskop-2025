#version 330 core

layout(location = 0) in vec2 inPos;   // pozicija verteksa u lokalnom sistemu (-0.5..0.5)
uniform vec2 uPos;                    // gde da ga pomerimo na ekranu (NDC)
uniform vec2 uScale;                  // skaliranje po x/y

void main()
{
    vec2 worldPos = inPos * uScale + uPos;
    gl_Position = vec4(worldPos, 0.0, 1.0);
}
