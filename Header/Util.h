#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

struct Color { float r, g, b, a; };

int endProgram(std::string message);
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned loadImageToTexture(const char* filePath);
GLFWcursor* loadImageToCursor(const char* filePath);

void updateViewport(int w, int h);
void createRectVAO(unsigned int& VAO, unsigned int& VBO);
void createTexturedQuadVAO(unsigned int& VAO, unsigned int& VBO);

void drawRect(unsigned int shader, unsigned int VAO,
    float cx, float cy, float sx, float sy, Color col);

void drawTexturedQuad(unsigned int shader, unsigned int VAO,
    unsigned int texture,
    float cx, float cy, float sx, float sy);

unsigned int createShaderSafe(const char* vsPath, const char* fsPath);
