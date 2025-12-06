#include "Cinema.h"
#include "Util.h"
#include "Seats.h"
#include "People.h"


#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>



void resetScene()
{
    overlayAlpha = 0.5f;
    projectionStarted = false;


    filmStarted = false;
    filmFinished = false;
    filmFrameCount = 0;

    screenColor = Color{ 1.0f, 1.0f, 1.0f, 1.0f };

    peopleLeaving = false;
}

void updateOverlayAndDoors(double now)
{
    
    if (projectionStarted && overlayAlpha > 0.0f)
    {
        overlayAlpha -= 0.01f;
        if (overlayAlpha < 0.0f) overlayAlpha = 0.0f;
    }

    if (doorOpening && doorOffset < 0.15f)
    {
        doorOffset += 0.002f;
        if (doorOffset > 0.15f) doorOffset = 0.15f;
    }

    if (doorClosing && doorOffset > 0.0f)
    {
        doorOffset -= 0.002f;
        if (doorOffset < 0.0f)
        {
            doorOffset = 0.0f;
            doorClosing = false;   
        }
    }
}

void updateFilm(double now)
{
    if (!filmStarted || filmFinished)
        return;

    double elapsed = now - filmStartTime;

    if (elapsed >= 20.0)
    {
        filmFinished = true;
        filmStarted = false;
        screenColor = Color{ 1,1,1,1 };

        peopleLeaving = true;
        doorOpening = true;
        doorClosing = false;

        double nowExit = now;
        const double EXIT_DELAY = 0.5;
        int exitIndex = 0;

        for (int i = (int)people.size() - 1; i >= 0; --i) {
            auto& p = people[i];
            if (p.stage == 2) {  
                p.stage = 3;     
                p.started = false;
                p.startTime = nowExit + exitIndex * EXIT_DELAY;
                exitIndex++;
            }
        }

        return;
    }


    if (filmFrameCount % 20 == 0)
    {
        float r = (rand() % 256) / 255.0f;
        float g = (rand() % 256) / 255.0f;
        float b = (rand() % 256) / 255.0f;

        screenColor = Color{ r, g, b, 1.0f };
    }
    filmFrameCount++;
}


    void drawScreenAndDoor()
    {
        float screenY = 0.70f;

        drawRect(rectShader, rectVAO,
            0.0f, screenY,
            1.0f, 0.60f,
            { 0.55f, 0.55f, 0.55f, 1.0f });

        drawRect(rectShader, rectVAO,
            0.0f, screenY,
            0.90f, 0.50f,
            screenColor);

        drawRect(rectShader, rectVAO,
            DOOR_CENTER_X, DOOR_CENTER_Y + 0.02f,
            0.18f, 0.40f,
            { 0.05f, 0.05f, 0.05f, 1.0f });

        drawRect(rectShader, rectVAO,
            DOOR_CENTER_X - doorOffset, DOOR_CENTER_Y,
            0.12f, 0.30f,
            { 0.15f, 0.35f, 0.18f, 1.0f });

        drawRect(rectShader, rectVAO,
            (DOOR_CENTER_X + 0.045f) - doorOffset, DOOR_CENTER_Y,
            0.012f, 0.012f,
            { 1.0f, 0.9f, 0.3f, 1.0f });
    }


