#include "Cinema.h"
#include "Util.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>

// Spolja definisane globalne promenljive (iz Main.cpp)
extern float overlayAlpha;
extern bool projectionStarted;

extern float doorOffset;
extern bool doorOpening;
extern bool doorClosing;

extern bool filmStarted;
extern bool filmFinished;
extern double filmStartTime;
extern int filmFrameCount;

extern Color screenColor;
extern bool peopleLeaving;

extern const float DOOR_CENTER_X;
extern const float DOOR_CENTER_Y;

extern unsigned int rectShader;
extern unsigned int rectVAO;

// ---------------------------------------------------------
// RESET CEO BIOSKOP: vraæa sve na poèetne vrednosti
// ---------------------------------------------------------
void resetScene()
{
    overlayAlpha = 0.5f;

    projectionStarted = false;

    doorOffset = 0.0f;
    doorOpening = false;
    doorClosing = false;

    filmStarted = false;
    filmFinished = false;
    filmFrameCount = 0;
    screenColor = Color{ 1,1,1,1 };

    peopleLeaving = false;
}

// ---------------------------------------------------------
// ANIMACIJA OVERLAY-a + VRATA
// ---------------------------------------------------------
void updateOverlayAndDoors(double now)
{
    // Nestajanje sivog overlay-a
    if (projectionStarted && overlayAlpha > 0.0f)
    {
        overlayAlpha -= 0.01f;
        if (overlayAlpha < 0.0f) overlayAlpha = 0.0f;
    }

    // Otvaranje vrata
    if (doorOpening && doorOffset < 0.15f)
    {
        doorOffset += 0.002f;
        if (doorOffset > 0.15f) doorOffset = 0.15f;
    }

    // Zatvaranje vrata
    if (doorClosing && doorOffset > 0.0f)
    {
        doorOffset -= 0.002f;
        if (doorOffset < 0.0f)
        {
            doorOffset = 0.0f;
            doorClosing = false;   // vrata potpuno zatvorena
        }
    }
}

// ---------------------------------------------------------
// FILM – menja boju ekrana svakih nekoliko frame-ova
// ---------------------------------------------------------
void updateFilm(double now)
{
    if (!filmStarted || filmFinished)
        return;

    double elapsed = now - filmStartTime;

    // Film traje 20 sekundi
    if (elapsed >= 20.0)
    {
        filmFinished = true;
        filmStarted = false;
        screenColor = Color{ 1,1,1,1 };

        // posle filma otvaramo vrata za izlazak
        peopleLeaving = true;
        doorOpening = true;
        doorClosing = false;
        return;
    }

    // Menjanje boja svakih 20 frejmova
    if (filmFrameCount % 20 == 0)
    {
        float r = (rand() % 256) / 255.0f;
        float g = (rand() % 256) / 255.0f;
        float b = (rand() % 256) / 255.0f;
        screenColor = Color{ r, g, b, 1.0f };
    }

    filmFrameCount++;
}

// ---------------------------------------------------------
// CRTANJE BIOSKOPSKOG PLATNA + VRATA
// ---------------------------------------------------------
void drawScreenAndDoor()
{
    float screenY = 0.70f;

    // ---- BIOSKOPSKO PLATNO (ram)
    drawRect(rectShader, rectVAO,
        0.0f, screenY,
        1.0f, 0.60f,
        { 0.55f, 0.55f, 0.55f, 1.0f });

    // ---- UNUTRAŠNJI PRIKAZ FILMA
    drawRect(rectShader, rectVAO,
        0.0f, screenY,
        0.90f, 0.50f,
        screenColor);

    // ---- NIŠA ZA VRATA
    drawRect(rectShader, rectVAO,
        DOOR_CENTER_X, DOOR_CENTER_Y + 0.02f,
        0.18f, 0.40f,
        { 0.05f, 0.05f, 0.05f, 1.0f });

    // ---- VRATA (pomeraju se ulevo)
    drawRect(rectShader, rectVAO,
        DOOR_CENTER_X - doorOffset, DOOR_CENTER_Y,
        0.12f, 0.30f,
        { 0.15f, 0.35f, 0.18f, 1.0f });

    // ---- KVAKA
    drawRect(rectShader, rectVAO,
        (DOOR_CENTER_X + 0.045f) - doorOffset, DOOR_CENTER_Y,
        0.012f, 0.012f,
        { 1.0f, 0.9f, 0.3f, 1.0f });
}
