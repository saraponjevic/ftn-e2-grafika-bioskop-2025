#pragma once
#include <vector>

struct Person {
    float x, y;
    float targetX, targetY;
    int stage;
    int seatIndex;
    double startTime;
    bool started;
};

extern std::vector<Person> people;

void spawnPeopleForProjection();   // pravi ljude kada se pritisne ENTER
void updatePeople(double now);     // pomera ih po fazama
void drawWalkingPeople();          // crtanje onih koji se kreæu
void drawSeatedPeople();           // crtanje onih koji sede

bool areAllSeated();
bool areAllExited();

