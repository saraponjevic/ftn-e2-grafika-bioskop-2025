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

float DOOR_CENTER_X = -0.92f;   //X koordinatu centra vrata - levo
float DOOR_CENTER_Y = 0.55f;  // gore
const float PERSON_SIZE = 0.10f;


// ljudi sedaju na mesta
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
        people.clear();  //obrisati staro
        return;
    }

    projectionStarted = true;
    doorOpening = true;

    people.clear();  

    int maxPeople = (int)allowed.size();  
    int numPeople = rand() % maxPeople + 1;  

    people.reserve(numPeople); 

    double now = glfwGetTime();  //od starta programa
    const double PERSON_DELAY = 0.5;

    for (int i = 0; i < numPeople; i++)
    {
        int randPos = rand() % allowed.size();   //nasumičan indeks
        int seatIndex = allowed[randPos];     
        allowed.erase(allowed.begin() + randPos);   //ne mogu dve osobe na 1 sediste

        Person p;
        p.x = DOOR_CENTER_X;  
        p.y = 1.5f;      //iznad scene pa silazi         
        p.targetX = seats[seatIndex].x;  
        p.targetY = seats[seatIndex].y;
        p.seatIndex = seatIndex;
        p.stage = 0; //prva faza ulaska
        p.started = false;
        p.startTime = now + i * PERSON_DELAY;   

        people.push_back(p);
    }
}



// kretanje ljudi u bioskopu
void updatePeople(double now)
{
    float speed = 0.005f;  // korak pomeranja

    for (auto& p : people)
    {
        if (!p.started)
        {
            if (now < p.startTime)
                continue;

            p.started = true;  //osoba je krenula

            if (!peopleLeaving) {   // pozicioniranje ispred vrata
                p.x = DOOR_CENTER_X;
                p.y = DOOR_CENTER_Y - 0.15f;
            }
        }

        // kretanje po y do reda sedista
        if (p.stage == 0)
        {
            if (fabs(p.y - p.targetY) > 0.01f)   
            {
                p.y += (p.y < p.targetY ? speed : -speed);
                //ko je ispod cilja - p.y += speed
                //ako je iznad cilja - p.y -= speed
            }
            else p.stage = 1; 
        }

        // kretanje po x do tacnog sedista
        else if (p.stage == 1)
        {
            if (fabs(p.x - p.targetX) > 0.01f)
            {
                p.x += (p.x < p.targetX ? speed : -speed);
            }
            else p.stage = 2; // sedi
        }

        // kretanje po x od sedista ka vratima
        else if (p.stage == 3)
        {
            if (fabs(p.x - DOOR_CENTER_X) > 0.01f)
            {
                p.x += (p.x < DOOR_CENTER_X ? speed : -speed);
            }
            else p.stage = 4;
        }

        // kretanje po y od vrata ka izlazu
        else if (p.stage == 4)
        {
            float exitY = DOOR_CENTER_Y - 0.15f;  //Y koordinata linije izlaza (ispod vrata)

            if (fabs(p.y - exitY) > 0.01f)
            {
                p.y += (p.y < exitY ? speed : -speed);
            }
            else p.stage = 5; // zavrsio izlazak
        }
    }
}


// crtanje ljudi koji se krecu
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
            p.x, p.y,   //trenutna poz osobe
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

        const Seat& s = seats[p.seatIndex];  //index sedišta u vektoru seats na kome ta osoba sedi
        //float seatScale = seatScaleForRow(p.seatIndex / 14);

        int row = p.seatIndex / NUM_COLS;  //daje red u kom je sedište
        float seatScale = seatScaleForRow(row);


        float seatH = SEAT_H * seatScale;  
        float seatTopY = s.y + seatH * 0.5f;  //Y koordinata vrha sedišta

        float drawX = s.x;   //(sedi na tom sedištu)
        float drawY = seatTopY + PERSON_SIZE * (-0.09f);  // da nije iznad sedista

        drawTexturedQuad(
            texShader, quadVAO, texStand,
            drawX, drawY,   //ka koji sedi
            PERSON_SIZE, PERSON_SIZE
        );
    }
}

// da li su svi ljudi seli na svoja sedista
bool areAllSeated()
{
    if (people.empty()) return false;  //Ako liste ljudi nema (niko nije ušao u salu)

    for (auto& p : people)
        if (p.stage != 2)
            return false;

    return true;
}


// da li su svi ljudi zavrsili izlazak iz sale
extern bool peopleLeaving;

bool areAllExited() {
    if (!peopleLeaving || people.empty()) return false;
    for (auto& p : people)
        if (p.stage != 5) return false;  //stanje 5 -osoba je završila izlazak iz sale
    return true;
}

