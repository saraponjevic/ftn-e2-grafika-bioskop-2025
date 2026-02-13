#pragma once
#include <glm/glm.hpp>

struct RoomDims {
    float W = 10.0f;   // sirina (x)
    float H = 4.0f;    // visina (y)
    float D = 14.0f;   // dubina (z)
};

void initRoom();
void drawRoom(unsigned int phongTexShader,
    const glm::mat4& view, const glm::mat4& proj,
    const glm::vec3& camPos);
