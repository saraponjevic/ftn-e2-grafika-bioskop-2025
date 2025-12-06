#pragma once
#include "Header/Util.h"



extern float overlayAlpha;
extern bool projectionStarted;
extern float doorOffset;
extern bool doorOpening;
extern bool doorClosing;

extern bool filmStarted;
extern bool filmFinished;
extern Color screenColor;
extern bool peopleLeaving;

extern double filmStartTime;
extern int filmFrameCount;
extern unsigned int rectShader;
extern unsigned int rectVAO;

extern float DOOR_CENTER_X;
extern float DOOR_CENTER_Y;

void resetScene();
void updateOverlayAndDoors(double now);
void updateFilm(double now);
void drawScreenAndDoor();
