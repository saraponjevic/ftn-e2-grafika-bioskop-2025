#include "Cinema.h"
#include "Util.h"
#include "Seats.h"
#include "People.h"


#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>



// funkcija koja na kraju kad svi izadju resetuje scenu
void resetScene()
{
    overlayAlpha = 0.5f;  
    projectionStarted = false;


    filmStarted = false; 
    filmFinished = false; 
    filmFrameCount = 0;  

    screenColor = Color{ 1.0f, 1.0f, 1.0f, 1.0f };  

    peopleLeaving = false; //da li ljudi napustaju salu
}


// tamnosivi pravougaonik preko ekrana + otvaranje i zatvaranje vrata -klizna vrata
void updateOverlayAndDoorss(double now)
{
    
   
    if (projectionStarted && overlayAlpha > 0.0f)  
    {
        overlayAlpha -= 0.01f;  //sve providniji
        if (overlayAlpha < 0.0f) overlayAlpha = 0.0f;
    }  


    //pocni da otvaras vrata
    if (doorOpening && doorOffset < 0.15f)
    {
        doorOffset += 0.002f;  
        if (doorOffset > 0.15f) doorOffset = 0.15f;  // ne preko max otvaranja
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


// boje platna + kad je kraj filma izlazak ljudi i platno opet belož

/*
void updateFilm(double now)
{
    if (!filmStarted || filmFinished)
        return;  

    double elapsed = now - filmStartTime;  

    if (elapsed >= 20.0) // gotov film
    {
        filmFinished = true;
        filmStarted = false;
        screenColor = Color{ 1.0f, 1.0f, 1.0f, 1.0f };

        peopleLeaving = true;
        doorOpening = true;
        doorClosing = false;

        double nowExit = now;  
        const double EXIT_DELAY = 0.5; 
        int exitIndex = 0;   //koliko ljudi je vec krenulo

        for (int i = (int)people.size() - 1; i >= 0; --i) {   //od posl 
            auto& p = people[i];   
            if (p.stage == 2) {   
                p.stage = 3;     
                p.started = false;   //nova animacija 
                p.startTime = nowExit + exitIndex * EXIT_DELAY; 
                exitIndex++;
            }
        }

        return;  
    }

    //menjanje boje na svakih 20 frejmova
    if (filmFrameCount % 20 == 0)
    {
        float r = (rand() % 256) / 255.0f;  
        float g = (rand() % 256) / 255.0f;
        float b = (rand() % 256) / 255.0f;

        screenColor = Color{ r, g, b, 1.0f };  
    }
    filmFrameCount++;
}*/


// rect.frag i rect.vert
void drawScreenAndDoor()
    {
        float screenY = 0.70f;  

        drawRect(rectShader, rectVAO,  //sivi pravougaonik oko platna
            0.0f, screenY,
            1.0f, 0.60f,
            { 0.55f, 0.55f, 0.55f, 1.0f });  

        drawRect(rectShader, rectVAO,  //platno -boja se menja tokom filma
            0.0f, screenY,
            0.90f, 0.50f,
            screenColor);

        
        drawRect(rectShader, rectVAO,   //okvir vrata sivi
            DOOR_CENTER_X, DOOR_CENTER_Y + 0.02f,
            0.18f, 0.40f,
            { 0.05f, 0.05f, 0.05f, 1.0f });

        
        drawRect(rectShader, rectVAO,    //pokretno krilo vrata
            DOOR_CENTER_X - doorOffset, DOOR_CENTER_Y,
            0.12f, 0.30f,
            { 0.15f, 0.35f, 0.18f, 1.0f });

       
        drawRect(rectShader, rectVAO,   //kvaka
            (DOOR_CENTER_X + 0.045f) - doorOffset, DOOR_CENTER_Y,
            0.012f, 0.012f,
            { 1.0f, 0.9f, 0.3f, 1.0f });
    }


