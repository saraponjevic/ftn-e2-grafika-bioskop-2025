#pragma once
#include <vector>
#include <glm/glm.hpp>

enum class SeatStatus { Free, Reserved, Bought };

struct Seat3D
{
    int row = 0;
    int col = 0;
    SeatStatus status = SeatStatus::Free;

    glm::vec3 pos = glm::vec3(0.0f);   // centar sedišta u svetu
    glm::vec3 half = glm::vec3(0.25f);  // polu-dimenzije (AABB)
    float yaw = 0.0f;                   // rotacija oko Y (radijani), da gleda ka platnu
};

struct Step3D
{
    glm::vec3 pos = glm::vec3(0.0f);   // centar stepenika
    glm::vec3 half = glm::vec3(1.0f);   // polu-dimenzije (AABB)
};

extern std::vector<Seat3D> seats3D;
extern std::vector<Step3D> steps3D;

// init
void initSeats3D();

// picking + state
int  pickSeat3D(float ndcX, float ndcY, const glm::mat4& view, const glm::mat4& proj);
void toggleReserveSeat3D(int idx);

// kupovina N uzastopnih (Free->Bought)
void buySeats3D(int N);

// helper za ekran (ako ti zatreba)
glm::vec3 getScreenPos3D();

