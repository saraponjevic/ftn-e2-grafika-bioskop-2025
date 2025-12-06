#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <cmath>
#include <thread>
#include <chrono>

#include "Util.h"
#include "People.h"
#include "Cinema.h"
#include "Seats.h"


// Main fajl funkcija sa osnovnim komponentama OpenGL programa

// Projekat je dozvoljeno pisati počevši od ovog kostura
// Toplo se preporučuje razdvajanje koda po fajlovima (i eventualno potfolderima) !!!
// Srećan rad!


void updateOverlayAndDoors(double now);
void updateFilm(double now);
void resetScene();
void drawScreenAndDoor();

extern const int   NUM_COLS;
extern const float SEAT_W;
extern const float SEAT_H;


float overlayAlpha = 0.5f;
bool projectionStarted = false;

float doorOffset = 0.0f;
bool doorOpening = false;
bool doorClosing = false;

bool filmStarted = false;
bool filmFinished = false;
double filmStartTime = 0.0;
int filmFrameCount = 0;

Color screenColor = { 1.0f, 1.0f, 1.0f, 1.0f };
bool peopleLeaving = false;

bool resetAfterDoorClose = false;


unsigned int rectShader, texShader;
unsigned int rectVAO, quadVAO;

unsigned int texWalkForward, texWalkLeft, texWalkRight, texStand;
unsigned int seatFreeTex, seatReservedTex, seatBoughtTex;

int screenWidth = 800, screenHeight = 800;

bool keyWasDown[GLFW_KEY_LAST + 1] = { false };


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float ndcX = (xpos / screenWidth) * 2.0f - 1.0f;
        float ndcY = -((ypos / screenHeight) * 2.0f - 1.0f);

        handleSeatClick(ndcX, ndcY);
    }
}


int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);
    screenWidth = mode->width;
    screenHeight = mode->height;



    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Bioskop", mon, nullptr);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    //gWindow = window;


    glfwSetMouseButtonCallback(window, mouse_button_callback);
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");



    glewInit();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    updateViewport(screenWidth, screenHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //crno

    GLFWcursor* cursor = loadImageToCursor("res/kursor.png");
    glfwSetCursor(window, cursor);


    rectShader = createShader("Shaders/rect.vert", "Shaders/rect.frag");
    texShader = createShader("Shaders/tex.vert", "Shaders/tex.frag");

   
   unsigned int VBO1, VBO2;
   createRectVAO(rectVAO, VBO1);
   createTexturedQuadVAO(quadVAO, VBO2);



    unsigned int nameTex = loadImageToTexture("res/me.png");
    glUseProgram(texShader);
    glUniform1i(glGetUniformLocation(texShader, "uTex"), 0);

    seatFreeTex = loadImageToTexture("res/plava.png");
    seatReservedTex = loadImageToTexture("res/zuta.png");
    seatBoughtTex = loadImageToTexture("res/crvena.png");



    texWalkForward = loadImageToTexture("res/pravo.png");
    texWalkLeft = loadImageToTexture("res/levo.png");
    texWalkRight = loadImageToTexture("res/desno.png");
    texStand = loadImageToTexture("res/sedenje.png");

   
    initSeats();


    const double FRAME_TIME = 1.0 / 75.0;



    while (!glfwWindowShouldClose(window))
    {
        double start = glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;

        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !projectionStarted)
        {
            spawnPeopleForProjection();
        }


        for (int key = GLFW_KEY_1; key <= GLFW_KEY_9; key++) {
            int state = glfwGetKey(window, key);
            if (state == GLFW_PRESS && !keyWasDown[key]) {
                buySeats(key - GLFW_KEY_0);
                keyWasDown[key] = true;
            }
            else if (state == GLFW_RELEASE) {
                keyWasDown[key] = false;
            }
        }

        updateOverlayAndDoors(start);
        updateFilm(start);
        updatePeople(start);



        bool allSeated = areAllSeated();
        bool allExited = areAllExited();

        if (allSeated && doorOffset >= 0.15f) {
            doorClosing = true;
            doorOpening = false;
        }

        if (allSeated && doorOffset <= 0.0f && !filmStarted && !filmFinished) {
            filmStarted = true;
            filmFinished = false;
            filmStartTime = start;
            filmFrameCount = 0;
            screenColor = Color{ 1.0f, 1.0f, 1.0f, 1.0f };
        }

        if (allExited) {
            resetScene();

            doorOpening = false;
            doorClosing = true;    

            people.clear();
            initSeats();            
        }



        glClear(GL_COLOR_BUFFER_BIT);

        drawScreenAndDoor();
        drawWalkingPeople();
       // drawSeatedPeople();   

        for (int i = 0; i < seats.size(); i++) {
            const Seat& s = seats[i];
            int row = i / NUM_COLS;
            float scale = seatScaleForRow(row);

            float w = SEAT_W * scale;
            float h = SEAT_H * scale;

            unsigned int tex =
                s.status == SeatStatus::Free ? seatFreeTex :
                s.status == SeatStatus::Reserved ? seatReservedTex :
                seatBoughtTex;

            drawTexturedQuad(texShader, quadVAO, tex,
                s.x, s.y, w, h);
        }

      //  drawWalkingPeople();
        drawSeatedPeople();


        drawRect(rectShader, rectVAO, 0.0f, 0.0f, 2.0f, 2.0f,
            { 0.1f, 0.1f, 0.1f, overlayAlpha });


        drawTexturedQuad(texShader, quadVAO, nameTex,
            -0.87f, -0.87f,   
            0.3f, 0.3f);    


        glfwSwapBuffers(window);
        glfwPollEvents();

        double dt = glfwGetTime() - start;
        if (dt < FRAME_TIME) {
            std::this_thread::sleep_for(std::chrono::duration<double>(FRAME_TIME - dt));
        }
    }

    glfwTerminate();
    return 0;
}
