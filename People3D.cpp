// ============================
// People3D.cpp
// ============================
#include "People3D.h"
#include "../Seat3D.h"   // ili "Seat3D.h" zavisi od putanje

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cmath>
#include <iostream>

#include <algorithm>


std::vector<Person3D> people3D;

extern std::vector<Seat3D> seats3D;


extern bool projectionStarted;
extern bool doorOpening;

extern float getStepsStartZ3D();

void startExitPeople3D(double now);
bool areAllExited3D();

static float PERSON_SPEED = 1.6f;
static const float GROUND_Y = 0.2f;
static const float FOOT = 0.22f;   // “visina stopala” / pivot fix

static const float EXIT_THROUGH_DOOR = 1.0f;   // koliko da proðe kroz vrata pre nestanka

static const float EXIT_CENTER_X_EPS = 0.03f;   // koliko blizu centra vrata pre prelaska kroz
static const float EXIT_EXTRA_TO_DOOR = 0.30f;  // da doðe malko bliže samim vratima


static float computeAisleX()
{
    float minX = seats3D.empty() ? -3.0f : seats3D[0].pos.x;
    for (auto& s : seats3D) minX = std::min(minX, s.pos.x);
    return minX - 1.25f;
}

static glm::vec3 computeEntryPos()
{
    glm::vec3 screen = getScreenPos3D();
    float aisleX = computeAisleX();
   // float y = 0.2f;
    float y = GROUND_Y + FOOT;

    float z = screen.z + 0.7f;
    return glm::vec3(aisleX, y, z);
}

static bool moveAxis(float& v, float target, float maxStep)
{
    float d = target - v;
    if (std::fabs(d) <= maxStep)
    {
        v = target;
        return true;
    }
    v += (d > 0 ? maxStep : -maxStep);
    return false;
}


static float floorTopY_atZ(float z)
{
    // default je pod (GROUND_Y)
    float best = GROUND_Y;

    // nadji stepenik èiji Z-opseg sadrži z i uzmi njegov "top"
    for (const auto& st : steps3D)
    {
        float z0 = st.pos.z - st.half.z;
        float z1 = st.pos.z + st.half.z;

        if (z >= z0 && z <= z1)
        {
            float top = st.pos.y + st.half.y;   // gornja ploha kocke
            if (top > best) best = top;
        }
    }
    return best;
}


void spawnPeopleForProjection3D()
{
    std::vector<int> allowed;

    glm::vec3 entry = computeEntryPos();
    std::cout << "ENTRY: " << entry.x << " " << entry.y << " " << entry.z << "\n";

    for (int i = 0; i < (int)seats3D.size(); i++)
    {
        if (seats3D[i].status == SeatStatus3D::Reserved || seats3D[i].status == SeatStatus3D::Bought)
        {
            allowed.push_back(i);
        }
    }

    if (allowed.empty())
    {
        projectionStarted = false;
        doorOpening = false;
        return;
    }

    projectionStarted = true;
    doorOpening = true;

    people3D.clear();

    int maxPeople = (int)allowed.size();
    int maxCap = std::min(15, maxPeople);
    int numPeople = 1 + (rand() % maxCap);

    double now = glfwGetTime();
    double DELAY = 0.25;

    for (int i = 0; i < numPeople; i++)
    {
        int r = rand() % allowed.size();
        int seatIdx = allowed[r];
        allowed.erase(allowed.begin() + r);

        Person3D p;
        p.pos = computeEntryPos();
        p.seatIndex = seatIdx;
        p.target = seats3D[seatIdx].pos;


        const Seat3D& s = seats3D[seatIdx];

        // forward vektor sedišta (ka platnu)
        glm::vec3 fwd = glm::normalize(glm::vec3(std::sin(s.yaw), 0.0f, std::cos(s.yaw)));

        // visina baze (isto kao u draw: baseHalf.y = s.half.y * 0.55f)
        float seatTopY = s.pos.y + (s.half.y * 0.55f);  // gornja površina sedalnog dela

        // malo ka naslonu (da ne sedi na ivici), ali NE previše
        p.target = s.pos - fwd * (s.half.z * 0.20f);

        // podigni ga da “legne” na sedalni deo (malo iznad)
        p.target.y = seatTopY + 0.02f;
        // gore na sedalni deo (NE dole)

        p.started = false;
        p.startTime = now + i * DELAY;
        p.stage = 0;
        p.modelIndex = i % 15;

        people3D.push_back(p);
        std::cout << "SPAWN people=" << people3D.size() << "\n";
    }
}

void updatePeople3D(float dt)
{
    double now = glfwGetTime();
    float step = PERSON_SPEED * dt;

    float stepsStartZ = getStepsStartZ3D();
    float groundY = 0.2f;

    for (auto& p : people3D)
    {
        if (!p.started)
        {
            if (now < p.startTime) continue;
            p.started = true;

            if (p.stage == 0) p.stage = 1;
        }

        if (p.stage == 1)
        {
            bool zDone = moveAxis(p.pos.z, p.target.z, step);

            float floorY = floorTopY_atZ(p.pos.z);
            float desiredY = floorY + FOOT + 0.10f;
            moveAxis(p.pos.y, desiredY, step * 0.90f);

            if (zDone) p.stage = 2;
        }
        else if (p.stage == 2)
        {
            bool xDone = moveAxis(p.pos.x, p.target.x, step);

            float floorY = floorTopY_atZ(p.pos.z);
            float desiredY = floorY + FOOT + 0.10f;
            moveAxis(p.pos.y, desiredY, step * 0.90f);

            if (xDone)
            {
                p.stage = 3;
                p.pos = p.target; // snap
            }
        }
        else if (p.stage == 3)
        {
            // sedi mirno
            moveAxis(p.pos.y, p.target.y, step * 0.6f);
            p.pos.x = p.target.x;
            p.pos.z = p.target.z;
        }
        else if (p.stage == 4)
        {
            float aisleX = computeAisleX();
            bool xDone = moveAxis(p.pos.x, aisleX, step);

            // dok izlazi boèno, drži ga na "podu/stepeniku" na njegovom z
            float floorY = floorTopY_atZ(p.pos.z);
            float desiredY = floorY + FOOT + 0.10f;
            moveAxis(p.pos.y, desiredY, step * 0.90f);

            if (xDone) p.stage = 5;
        }
        else if (p.stage == 5)
        {
            glm::vec3 entry = computeEntryPos();

            // smer ka izlazu (iz sedišta ka entry)
            float seatZ = (p.seatIndex >= 0 && p.seatIndex < (int)seats3D.size())
                ? seats3D[p.seatIndex].pos.z
                : p.pos.z;

            float sign = (entry.z > seatZ) ? 1.0f : -1.0f;

            // cilj je "iza vrata" (kroz vrata)
            float exitZ = entry.z + sign * EXIT_THROUGH_DOOR;

            // 1) UVEK guraj Z ka exitZ (da stvarno proðe kroz)
            bool zDone = moveAxis(p.pos.z, exitZ, step);

            // 2) X poravnanje radi usput (ne blokira kretanje po Z)
            moveAxis(p.pos.x, entry.x, step * 0.85f);

            // visina po podu/stepeniku
            float floorY = floorTopY_atZ(p.pos.z);
            float desiredY = floorY + FOOT + 0.08f;
            moveAxis(p.pos.y, desiredY, step * 0.90f);

            if (zDone)
            {
                p.stage = 6;
                p.started = false; // nestane tek kad prodje "iza vrata"
            }
        }


        

    }

}

bool areAllSeated3D()
{
    if (people3D.empty()) return false;
    for (auto& p : people3D)
        if (p.stage != 3) return false;
    return true;
}

void startExitPeople3D(double now)
{
    if (people3D.empty()) return;

    const double EXIT_DELAY = 0.25;
    int k = 0;

    for (int i = (int)people3D.size() - 1; i >= 0; --i)
    {
        auto& p = people3D[i];
        if (p.stage == 3)
        {
            p.started = false;
            p.startTime = now + k * EXIT_DELAY;
            p.stage = 4;
            k++;
        }
    }
}

bool areAllExited3D()
{
    if (people3D.empty()) return false;
    for (auto& p : people3D)
        if (p.stage != 6) return false;
    return true;
}

float getAisleX3D() { return computeAisleX(); }
glm::vec3 getEntryPos3D() { return computeEntryPos(); }
