#include "People.h"
#include "Seats.h"
#include "Cinema.h"
#include "Header/Util.h"

#include <vector>
#include <cmath>
#include <cstdlib>
#include <GL/glew.h>
#include <GLFW/glfw3.h>


std::vector<Person> people;

float DOOR_CENTER_X = -0.92f;
float DOOR_CENTER_Y = 0.55f;
const float PERSON_SIZE = 0.10f;


void spawnPeopleForProjection()
{
    std::vector<int> allowed;
    for (int i = 0; i < (int)seats.size(); i++) {
        if (seats[i].status == SeatStatus::Reserved ||
            seats[i].status == SeatStatus::Bought) {
            allowed.push_back(i);
        }
    }

    if (allowed.empty()) {
        projectionStarted = false;
        doorOpening = false;
        people.clear();
        return;
    }

    projectionStarted = true;
    doorOpening = true;

    people.clear();

    int maxPeople = (int)allowed.size();
    int numPeople = rand() % maxPeople + 1;  

    people.reserve(numPeople);

    double now = glfwGetTime();
    const double PERSON_DELAY = 0.5;

    for (int i = 0; i < numPeople; i++)
    {
        int randPos = rand() % allowed.size();
        int seatIndex = allowed[randPos];
        allowed.erase(allowed.begin() + randPos);

        Person p;
        p.x = DOOR_CENTER_X;
        p.y = 1.5f;             
        p.targetX = seats[seatIndex].x;
        p.targetY = seats[seatIndex].y;
        p.seatIndex = seatIndex;
        p.stage = 0;
        p.started = false;
        p.startTime = now + i * PERSON_DELAY;

        people.push_back(p);
    }
}




void updatePeople(double now)
{
    float speed = 0.005f;

    for (auto& p : people)
    {
        if (!p.started)
        {
            if (now < p.startTime)
                continue;

            p.started = true;

            if (!peopleLeaving) {
                p.x = DOOR_CENTER_X;
                p.y = DOOR_CENTER_Y - 0.15f;
            }
        }

        if (p.stage == 0)
        {
            if (fabs(p.y - p.targetY) > 0.01f)
            {
                p.y += (p.y < p.targetY ? speed : -speed);
            }
            else p.stage = 1;
        }

        else if (p.stage == 1)
        {
            if (fabs(p.x - p.targetX) > 0.01f)
            {
                p.x += (p.x < p.targetX ? speed : -speed);
            }
            else p.stage = 2; // sedi
        }

        else if (p.stage == 3)
        {
            if (fabs(p.x - DOOR_CENTER_X) > 0.01f)
            {
                p.x += (p.x < DOOR_CENTER_X ? speed : -speed);
            }
            else p.stage = 4;
        }

        else if (p.stage == 4)
        {
            float exitY = DOOR_CENTER_Y - 0.15f;

            if (fabs(p.y - exitY) > 0.01f)
            {
                p.y += (p.y < exitY ? speed : -speed);
            }
            else p.stage = 5; // završio izlazak
        }
    }
}


void drawWalkingPeople()
{
    extern unsigned int quadVAO;
    extern unsigned int texShader;

    for (auto& p : people)
    {
        if (!p.started) continue;
        if (p.stage == 2 || p.stage == 5) continue;

        unsigned int tex;

        if (p.stage == 0 || p.stage == 4)
            tex = texWalkForward;
        else
        {
            float targetX = (p.stage == 1 ? p.targetX : DOOR_CENTER_X);
            tex = (p.x < targetX ? texWalkRight : texWalkLeft);
        }

        drawTexturedQuad(
            texShader, quadVAO, tex,
            p.x, p.y,
            PERSON_SIZE, PERSON_SIZE
        );
    }
}


void drawSeatedPeople()
{
    extern unsigned int quadVAO;
    extern unsigned int texShader;

    for (auto& p : people)
    {
        if (p.stage != 2) continue;

        const Seat& s = seats[p.seatIndex];
        //float seatScale = seatScaleForRow(p.seatIndex / 14);

        int row = p.seatIndex / NUM_COLS;
        float seatScale = seatScaleForRow(row);


        float seatH = SEAT_H * seatScale;
        float seatTopY = s.y + seatH * 0.5f;

        float drawX = s.x;
        float drawY = seatTopY + PERSON_SIZE * (-0.09f);

        drawTexturedQuad(
            texShader, quadVAO, texStand,
            drawX, drawY,
            PERSON_SIZE, PERSON_SIZE
        );
    }
}


bool areAllSeated()
{
    if (people.empty()) return false;

    for (auto& p : people)
        if (p.stage != 2)
            return false;

    return true;
}


extern bool peopleLeaving;

bool areAllExited() {
    if (!peopleLeaving || people.empty()) return false;
    for (auto& p : people)
        if (p.stage != 5) return false;
    return true;
}

