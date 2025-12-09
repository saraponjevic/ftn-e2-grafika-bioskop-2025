#version 330 core

out vec4 outCol;  

uniform vec4 uColor;   // boja + alpha

void main()
{
    outCol = uColor;
}
