#pragma once
#include <vector>
#include "Seats.h"   
#include "Util.h"   

struct Person {
    float x, y;
    float targetX, targetY;
    int stage;
    int seatIndex;
    double startTime;
    bool started;
};

extern std::vector<Person> people;

extern unsigned int texWalkForward;
extern unsigned int texWalkLeft;
extern unsigned int texWalkRight;
extern unsigned int texStand;

extern bool projectionStarted;
extern bool doorOpening;

extern std::vector<Seat> seats;
extern bool peopleLeaving;

extern const float PERSON_SIZE;


extern float DOOR_CENTER_X;
extern float DOOR_CENTER_Y;

void spawnPeopleForProjection();  
void updatePeople(double now);   
void drawWalkingPeople();        
void drawSeatedPeople();           

bool areAllSeated();
bool areAllExited();

