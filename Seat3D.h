#pragma once
#include <vector>
#include <glm/glm.hpp>

enum class SeatStatus3D { Free, Reserved, Bought };

struct Seat3D
{
    int row = 0;
    int col = 0;

    SeatStatus3D status = SeatStatus3D::Free;

    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 half = glm::vec3(0.25f);
    float yaw = 0.0f;
};

struct Step3D
{
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 half = glm::vec3(1.0f);
};

extern std::vector<Seat3D> seats3D;
extern std::vector<Step3D> steps3D;

void initSeats3D();

int  pickSeat3D(float ndcX, float ndcY, const glm::mat4& view, const glm::mat4& proj);
void toggleReserveSeat3D(int idx);

void buySeats3D(int N);

glm::vec3 getScreenPos3D();

float getStepsStartZ3D();
