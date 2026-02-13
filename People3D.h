// People3D.h
#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Person3D {
    glm::vec3 pos{ 0.0f };
    glm::vec3 target{ 0.0f };
    int seatIndex = -1;

    bool started = false;
    double startTime = 0.0;

    int stage = 0;
    int modelIndex = 0;

  

    // 0 wait/start
    // 1 goZ_to_row (krece se ka target.z, y prati visinu reda)
    // 2 goX_to_seat
    // 3 seated
    // 4 exit_goX_to_aisle
    // 5 exit_goZ_to_door
    // 6 exited
};

extern std::vector<Person3D> people3D;

void spawnPeopleForProjection3D();
void updatePeople3D(float dt);

bool areAllSeated3D();
bool areAllExited3D();
void startExitPeople3D(double now);

float getAisleX3D();
glm::vec3 getEntryPos3D();



