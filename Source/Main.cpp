// ============================
// main.cpp
// ============================
#define _CRT_SECURE_NO_WARNINGS

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdio>

#include "Util.h"
#include "People.h"
#include "Cinema.h"
#include "Seats.h"
#include "ModelSimple.h"

#include <direct.h>   // _getcwd
#include <crtdbg.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Seat3D.h"
#include "People3D.h"
#include "../Room.h"

// Main fajl funkcija sa osnovnim komponentama OpenGL programa

//void updateOverlayAndDoors(double now);
void updateFilm(double now);
void resetScene();
void drawScreenAndDoor();
void startExitPeople3D(double now);

unsigned int whiteScreenTex = 0;

unsigned int crosshairVAO = 0, crosshairVBO = 0;
unsigned int colorShader = 0;

extern const int NUM_COLS;
extern const float SEAT_W;
extern const float SEAT_H;

glm::vec3 camPos = glm::vec3(0.0f, 1.35f, 4.6f);
glm::vec3 camFront = glm::vec3(0.0f, -0.20f, -1.0f);
glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);

// film
std::vector<unsigned int> filmFrames;
int currentFilmFrame = 0;
double filmFPS = 12.0;

// overlay + state
float overlayAlpha = 0.5f;
bool projectionStarted = false;

static const float DOOR_OVERLAP_Z = 0.06f;   // koliko krilo iskoči ka sali (preko okvira)
static const float DOOR_GAP_Z = 0.01f;   // mala razdaljina da ne seče okvir (anti z-fighting)




bool filmStarted = false;
bool filmFinished = false;
double filmStartTime = 0.0;
int filmFrameCount = 0; //broj frejmova tokom filma

Color screenColor = { 1.0f, 1.0f, 1.0f, 1.0f }; //belo

bool peopleLeaving = false;


unsigned int rectShader, texShader;
unsigned int rectVAO, quadVAO;

unsigned int texWalkForward, texWalkLeft, texWalkRight, texStand;
unsigned int seatFreeTex, seatReservedTex, seatBoughtTex;

int screenWidth = 800, screenHeight = 800;

// input
bool keyWasDown[GLFW_KEY_LAST + 1] = { false };

bool gDepthOn = true;     // posto ga palis na startu
bool gCullOn = false;    // posto ga gasis na startu


// camera mouse look
float yaw = -90.0f;   // gleda ka -Z
float pitch = 0.0f;
double lastX = 0.0, lastY = 0.0;
bool firstMouse = true;

float mouseSensitivity = 0.12f;
float moveSpeed = 3.0f; // jedinice u sekundi

// GLOBAL view/proj matrice (mora iznad callback-a!)
glm::mat4 gView(1.0f);
glm::mat4 gProj(1.0f);






// ===== DOOR 3D params =====
glm::vec3 gDoorCenter = glm::vec3(0.0f, 1.0f, 0.0f); // postavicemo posle iz Room dims / entrance
glm::vec3 gDoorHalf = glm::vec3(0.35f, 0.90f, 0.05f); // polu-dimenzije krila
glm::vec3 gDoorFrameHalf = glm::vec3(0.75f, 1.00f, 0.08f);

float doorOffset = 0.0f;     // 0..DOOR_MAX
bool doorOpening = false;
bool doorClosing = false;

static const float DOOR_MAX = 0.90f;   // ili 1.0f ako hoces jos vise
// koliko se krilo pomeri levo/desno
static const float DOOR_SPEED = 1.2f;      // jedinice u sekundi (podesi)
bool resetAfterDoorClose = false;



void updateDoors(float dt)
{
    if (doorOpening)
    {
        doorOffset += DOOR_SPEED * dt;
        if (doorOffset >= DOOR_MAX)
        {
            doorOffset = DOOR_MAX;
            doorOpening = false;
        }
    }

    if (doorClosing)
    {
        doorOffset -= DOOR_SPEED * dt;
        if (doorOffset <= 0.0f)
        {
            doorOffset = 0.0f;
            doorClosing = false;

            // Ako smo cekali reset posle zatvaranja:
            if (resetAfterDoorClose)
            {
                resetAfterDoorClose = false;
                resetScene();
                people3D.clear();
                initSeats3D();

                projectionStarted = false;
                filmStarted = false;
                filmFinished = false;

            }
        }
    }
}






void loadFilmFrames()
{
    filmFrames.clear();
    const int N = 21;
    for (int i = 0; i < N; i++)
    {
        char path[256];
        std::snprintf(path, sizeof(path), "res/f%d.jpg", i + 1);
        unsigned int tex = loadImageToTexture(path);
        if (tex != 0) filmFrames.push_back(tex);
    }
}

void updateFilm(double now)
{
    if (!filmStarted || filmFrames.empty()) return;

    // FILM TRAJE TACNO 20s
    const double FILM_DURATION = 20.0;
    double t = now - filmStartTime;

    // ako je proslo 20s -> kraj
    if (t >= FILM_DURATION)
    {
        filmFinished = true;
        filmStarted = false;

        // platno postaje belo
        if (whiteScreenTex != 0) setScreenTexture(whiteScreenTex);

        // vrata se otvore
        doorOpening = true;
        doorClosing = false;

        // ljudi krecu da izlaze ISTOM putanjom (reverse)
        startExitPeople3D(now);
        return;
    }

    // u toku filma: teksture se smenjuju
    int N = (int)filmFrames.size();
    int idx = (int)((t / FILM_DURATION) * N);
    if (idx < 0) idx = 0;
    if (idx >= N) idx = N - 1;
    currentFilmFrame = idx;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (projectionStarted) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
        std::cout << "FB size: " << fbW << "x" << fbH << "\n";

        // centar ekrana (tamo gde je krstić)
        double xpos = fbW * 0.5;
        double ypos = fbH * 0.5;

        float ndcX = (float)(xpos / fbW) * 2.0f - 1.0f;
        float ndcY = 1.0f - (float)(ypos / fbH) * 2.0f;

        int idx = pickSeat3D(ndcX, ndcY, gView, gProj);
        std::cout << "CLICK idx=" << idx << "\n";
        toggleReserveSeat3D(idx);
    }
}

void mouse_look_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; // obrnuto jer je y na ekranu na dole
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += (float)xoffset;
    pitch += (float)yoffset;

    // limit pitch da ne “prevrne” kameru
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(dir);
}

void initCrosshair()
{
    float size = 0.02f;
    float verts[] = {
        // horizontalna linija
        -size, 0.0f, 0.0f,
         size, 0.0f, 0.0f,
         // vertikalna linija
          0.0f, -size, 0.0f,
          0.0f,  size, 0.0f
    };

    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);

    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void drawCrosshair()
{
    glUseProgram(colorShader);

    glm::mat4 mvp = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(colorShader, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3f(glGetUniformLocation(colorShader, "uColor"), 1.0f, 0.6f, 0.95f);

    glBindVertexArray(crosshairVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);

    glUseProgram(0);
}



void drawMeBottomLeft(GLFWwindow* window, unsigned int tex)
{
    if (!tex) return;

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    // HUD u pikselima
    float margin = 20.0f;
    float sizePx = 140.0f; // menjaj po želji (npr 100-200)

    // centar kvadrata u pikselima (donji levi)
    float cx = margin + sizePx * 0.5f;
    float cy = margin + sizePx * 0.5f;

    // pretvori pix -> NDC (-1..1)
    float cxN = (cx / fbW) * 2.0f - 1.0f;
    float cyN = (cy / fbH) * 2.0f - 1.0f;

    float sxN = (sizePx / fbW) * 2.0f;
    float syN = (sizePx / fbH) * 2.0f;

    glUseProgram(texShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(texShader, "uTex"), 0);

    // drawTexturedQuad očekuje centar + skale u NDC
    drawTexturedQuad(texShader, quadVAO, tex, cxN, cyN, sxN, syN);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}


// Pomoc: iz direction vektora postavi yaw/pitch + camFront
static void setLookDirFromVector(const glm::vec3& dirNormalized)
{
    pitch = glm::degrees(asinf(glm::clamp(dirNormalized.y, -1.0f, 1.0f)));
    yaw = glm::degrees(atan2f(dirNormalized.z, dirNormalized.x)) - 90.0f;
    camFront = dirNormalized;
    firstMouse = true;
}

// ===============================
// CAMERA CLAMP TO ROOM
// ===============================
static void clampCameraToRoom()
{
    RoomDims rd = getRoomDims();

    // Pretpostavka: soba je centrirana u (0,*,0)
    // X ide od -W/2 do +W/2
    // Z ide od -D/2 do +D/2
    // Y ide od 0 do H  (pod na 0)

    const float margin = 0.20f; // da ne "seče" zidove
    const float headMargin = 0.10f;

    float minX = -rd.W * 0.5f + margin;
    float maxX = rd.W * 0.5f - margin;

    float minZ = -rd.D * 0.5f + margin;
    float maxZ = rd.D * 0.5f - margin;

    float minY = 0.10f;                 // malo iznad poda
    float maxY = rd.H - headMargin;     // malo ispod plafona

    camPos.x = glm::clamp(camPos.x, minX, maxX);
    camPos.z = glm::clamp(camPos.z, minZ, maxZ);
    camPos.y = glm::clamp(camPos.y, minY, maxY);
}


// Postavi kameru iza poslednjeg reda i okreni ka platnu/centru sale
static void placeCameraBehindLastRow()
{
    if (seats3D.empty()) return;

    float minX = seats3D[0].pos.x, maxX = seats3D[0].pos.x;
    float minZ = seats3D[0].pos.z, maxZ = seats3D[0].pos.z;
    float avgY = 0.0f;

    for (const auto& s : seats3D)
    {
        minX = std::min(minX, s.pos.x);
        maxX = std::max(maxX, s.pos.x);
        minZ = std::min(minZ, s.pos.z);
        maxZ = std::max(maxZ, s.pos.z);
        avgY += s.pos.y;
    }
    avgY /= (float)seats3D.size();

    float centerX = 0.5f * (minX + maxX);

    bool screenIsTowardsMinusZ = true;
    float backZ = screenIsTowardsMinusZ ? maxZ : minZ;

    float behind = 1.6f;
    float eyeH = 1.35f;
    float camZ = screenIsTowardsMinusZ ? (backZ + behind) : (backZ - behind);

    camPos = glm::vec3(centerX, avgY + eyeH, camZ);

    float frontZ = screenIsTowardsMinusZ ? minZ : maxZ;
    glm::vec3 target = glm::vec3(centerX, 1.0f, frontZ);

    glm::vec3 dir = glm::normalize(target - camPos);
    setLookDirFromVector(dir);
}

extern std::vector<Person3D> people3D;

int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtCheckMemory();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);



    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);

    // uzmi rezoluciju monitora
    screenWidth = mode->width;
    screenHeight = mode->height;

    // napravi FULLSCREEN (mon umesto nullptr)
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Bioskop", mon, nullptr);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");



    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");

    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_look_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");
    glewInit();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    updateViewport(screenWidth, screenHeight);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    rectShader = createShader("Shaders/rect.vert", "Shaders/rect.frag");
    texShader = createShader("Shaders/tex.vert", "Shaders/tex.frag");
    unsigned int phongShader = createShaderSafe("Shaders/phong.vert", "Shaders/phong.frag");

    std::vector<std::unique_ptr<ModelSimple>> humans;
    const int MAX_HUMANS = 2;
    humans.reserve(MAX_HUMANS);


    const char* humanPaths[15] = {
        "res/human/h1/model_mesh.obj",
        "res/human/h2/model_mesh.obj",
        "res/human/h3/model_mesh.obj",
        "res/human/h4/model_mesh.obj",
        "res/human/h5/model_mesh.obj",
        "res/human/h6/model_mesh.obj",
        "res/human/h7/model_mesh.obj",
        "res/human/h8/model_mesh.obj",
        "res/human/h9/model_mesh.obj",
        "res/human/h10/model_mesh.obj",
        "res/human/h11/model_mesh.obj",
        "res/human/h12/model_mesh.obj",
        "res/human/h13/model_05_color.obj",
        "res/human/h14/model_mesh.obj",
        "res/human/h15/model_mesh.obj"
    };

    char cwd[1024];
    if (_getcwd(cwd, sizeof(cwd))) std::cout << "CWD=" << cwd << "\n";
    else std::cout << "CWD=?\n";

    for (int i = 0; i < MAX_HUMANS; i++)
    {
        std::cout << "Loading human: " << humanPaths[i] << "\n";
        humans.push_back(std::make_unique<ModelSimple>());

        bool ok = humans.back()->load(humanPaths[i]);
        std::cout << " -> load result: " << ok << "\n";

        if (!ok) humans.pop_back();
    }
    std::cout << "Humans loaded: " << humans.size() << "\n";

    unsigned int phongTexShader = createShaderSafe("Shaders/phong_tex.vert", "Shaders/phong_tex.frag");

    colorShader = createShaderSafe("Shaders/color2d.vert", "Shaders/color2d.frag");
    initCrosshair();

    initRoom();




    RoomDims rd = getRoomDims();

   // float zFront = rd.D * 0.5f;        // FRONT wall je na +D/2 (z1)
    float doorW = 1.7f;
    float doorH = 2.2f;

    float xDoor = -rd.W * 0.45f;       // isto kao u Room.cpp (doorCX)
    float yDoor = doorH * 0.5f;        // centar po visini (1.1)
    float zBack = -rd.D * 0.5f;                 // BACK wall (kod platna)
    gDoorCenter = glm::vec3(xDoor, yDoor, zBack + 0.02f); // malo “unutra” u salu
    // malo unutra u salu
    // malo "unutra" u salu

  //  gDoorCenter = glm::vec3(xDoor, yDoor, zFront - 0.02f); // malo u zid
// malo veca krila (sirina/visina), ali ne da probiju okvir
    gDoorHalf = glm::vec3(doorW * 0.30f, doorH * 0.50f, 0.035f);

    // krilo: W/2 pa pola => W/4
    gDoorFrameHalf = glm::vec3(doorW * 0.5f, doorH * 0.5f + 0.1f, 0.08f);







    // Vrata stavi na "ulaz" (front wall = z1 iz sobe).
// Ako ti je platno na z0 (kao u Room.cpp), onda je ulaz na z1.
// dims nisu javne kod tebe, pa uzimamo screenPos i idemo ka "publika" strani.
   // glm::vec3 screenPos = getScreenPos3D();
  //  gDoorCenter = glm::vec3(0.0f, 1.0f, screenPos.z + 0.75f); // ovo je isti fazon kao computeEntryPos()



















    loadFilmFrames();
    whiteScreenTex = loadImageToTexture("res/scrr.jpg");

    glDisable(GL_CULL_FACE);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 3.0f),
        glm::vec3(0.0f, 0.8f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    glm::mat4 proj = glm::perspective(glm::radians(70.0f), (float)fbW / (float)fbH, 0.1f, 100.0f);

    glUseProgram(phongShader);
    int uMLoc = glGetUniformLocation(phongShader, "uM");
    int uVLoc = glGetUniformLocation(phongShader, "uV");
    int uPLoc = glGetUniformLocation(phongShader, "uP");
    int uViewPosLoc = glGetUniformLocation(phongShader, "uViewPos");

    // Light 0
    int uLight0PosLoc = glGetUniformLocation(phongShader, "uLights[0].pos");
    int uLight0ALoc = glGetUniformLocation(phongShader, "uLights[0].kA");
    int uLight0DLoc = glGetUniformLocation(phongShader, "uLights[0].kD");
    int uLight0SLoc = glGetUniformLocation(phongShader, "uLights[0].kS");

    // Light 1
    int uLight1PosLoc = glGetUniformLocation(phongShader, "uLights[1].pos");
    int uLight1ALoc = glGetUniformLocation(phongShader, "uLights[1].kA");
    int uLight1DLoc = glGetUniformLocation(phongShader, "uLights[1].kD");
    int uLight1SLoc = glGetUniformLocation(phongShader, "uLights[1].kS");

    int uMatShineLoc = glGetUniformLocation(phongShader, "uMaterial.shine");
    int uMatALoc = glGetUniformLocation(phongShader, "uMaterial.kA");
    int uMatDLoc = glGetUniformLocation(phongShader, "uMaterial.kD");
    int uMatSLoc = glGetUniformLocation(phongShader, "uMaterial.kS");

    glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uVLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uPLoc, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3f(uViewPosLoc, 0.0f, 1.0f, 3.0f);

    glUniform3f(uLight0PosLoc, 0.0f, 2.0f, 2.0f);
    glUniform3f(uLight0ALoc, 0.2f, 0.2f, 0.2f);
    glUniform3f(uLight0DLoc, 0.8f, 0.8f, 0.8f);
    glUniform3f(uLight0SLoc, 1.0f, 1.0f, 1.0f);

    glUniform3f(uLight1PosLoc, -2.0f, 1.0f, 1.5f);
    glUniform3f(uLight1ALoc, 0.1f, 0.1f, 0.1f);
    glUniform3f(uLight1DLoc, 0.4f, 0.4f, 0.4f);
    glUniform3f(uLight1SLoc, 0.6f, 0.6f, 0.6f);

    glUniform1f(uMatShineLoc, 64.0f);
    glUniform3f(uMatALoc, 0.2f, 0.2f, 0.2f);
    glUniform3f(uMatDLoc, 0.7f, 0.2f, 0.2f);
    glUniform3f(uMatSLoc, 1.0f, 1.0f, 1.0f);
    glUseProgram(0);

    unsigned int VBO1, VBO2;
    createRectVAO(rectVAO, VBO1);
    createTexturedQuadVAO(quadVAO, VBO2);

  //  unsigned int nameTex = loadImageToTexture("res/me.png");


    float vertices[] =
    {   //Kocka
        //Normale su potrebne za racun osvjetljenja.
    //X     Y      Z       NX    NY     NZ
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };


    unsigned int stride = (3 + 3) * sizeof(float);
    unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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

    initSeats3D();
    placeCameraBehindLastRow();

    const double FRAME_TIME = 1.0 / 75.0;

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        static double lastTime = glfwGetTime();
        double now = glfwGetTime();
        float dtCam = (float)(now - lastTime);




        updateDoors(dtCam);





        lastTime = now;

        float v = moveSpeed * dtCam;
        glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camPos += camFront * v;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camPos -= camFront * v;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camPos -= right * v;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camPos += right * v;

        clampCameraToRoom();


        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        double start = glfwGetTime();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        int enterState = glfwGetKey(window, GLFW_KEY_ENTER);
        if (enterState == GLFW_PRESS && !keyWasDown[GLFW_KEY_ENTER] && !projectionStarted)
        {
            keyWasDown[GLFW_KEY_ENTER] = true;
            filmStarted = false;
            filmFinished = false;

            doorOpening = true;
            doorClosing = false;


            spawnPeopleForProjection3D();
            projectionStarted = true;
        }
        else if (enterState == GLFW_RELEASE)
        {
            keyWasDown[GLFW_KEY_ENTER] = false;
        }

        // kupovina karata - samo pre Enter (dok projekcija nije krenula)
        if (!projectionStarted)
        {
            for (int key = GLFW_KEY_1; key <= GLFW_KEY_9; key++)
            {
                int state = glfwGetKey(window, key);
                if (state == GLFW_PRESS && !keyWasDown[key])
                {
                    buySeats3D(key - GLFW_KEY_0);
                    keyWasDown[key] = true;
                }
                else if (state == GLFW_RELEASE)
                {
                    keyWasDown[key] = false;
                }
            }
        }

        // ===============================
// TOGGLE DEPTH TEST (T) + CULL (C)
// ===============================

// T - depth ON/OFF
        int tState = glfwGetKey(window, GLFW_KEY_T);
        if (tState == GLFW_PRESS && !keyWasDown[GLFW_KEY_T])
        {
            keyWasDown[GLFW_KEY_T] = true;
            gDepthOn = !gDepthOn;

            if (gDepthOn) glEnable(GL_DEPTH_TEST);
            else          glDisable(GL_DEPTH_TEST);

            std::cout << "[TOGGLE] Depth test: " << (gDepthOn ? "ON" : "OFF") << "\n";
        }
        else if (tState == GLFW_RELEASE)
        {
            keyWasDown[GLFW_KEY_T] = false;
        }

        // C - cull ON/OFF
        int cState = glfwGetKey(window, GLFW_KEY_C);
        if (cState == GLFW_PRESS && !keyWasDown[GLFW_KEY_C])
        {
            keyWasDown[GLFW_KEY_C] = true;
            gCullOn = !gCullOn;

            if (gCullOn)
            {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);
            }
            else
            {
                glDisable(GL_CULL_FACE);
            }

            std::cout << "[TOGGLE] Cull face: " << (gCullOn ? "ON" : "OFF") << "\n";
        }
        else if (cState == GLFW_RELEASE)
        {
            keyWasDown[GLFW_KEY_C] = false;
        }


       // updateOverlayAndDoors(start);
        updateFilm(start);

        if (filmStarted && !filmFrames.empty())
        {
            setScreenTexture(filmFrames[currentFilmFrame]);
        }

        updatePeople3D(dtCam);

        bool allSeated = areAllSeated3D();
        bool allExited = areAllExited3D();

        if (allSeated && doorOffset >= DOOR_MAX)
        {
            doorClosing = true;
            doorOpening = false;
        }

        if (projectionStarted && allSeated && doorOffset <= 0.0f && !filmStarted && !filmFinished)
        {
            filmStarted = true;
            filmFinished = false;
            filmStartTime = start;
            filmFrameCount = 0;
            screenColor = Color{ 1.0f, 1.0f, 1.0f, 1.0f };
        }


        if (allExited && !resetAfterDoorClose)
        {
            // prvo otvori (ako vec nije) - u praksi je otvoreno, ali nek bude sigurno
            doorOpening = false;
            doorClosing = true;

            resetAfterDoorClose = true; // reset ce se desiti kad doorOffset padne na 0
        }


        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        // mrak kad nema ljudi ili kad traje film
        bool emptyHall2 = allExited || people3D.empty();
        bool dark = emptyHall2 || filmStarted;

        if (dark) glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
        else      glClearColor(0.20f, 0.20f, 0.20f, 1.0f);

       


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        model = glm::mat4(1.0f);

        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glm::mat4 proj = glm::perspective(glm::radians(70.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

        gView = view;
        gProj = proj;

        glUseProgram(phongShader);
        glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uVLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uPLoc, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(uViewPosLoc, 1, glm::value_ptr(camPos));

        // >>> OVDE TACNO <<<
      // >>> OVDE TACNO <<<

// 1) logika svetla
      // ===== LOGIKA SVETLA =====

// 1) da li uopšte ima "projekcije u toku" (tj. već su ljudi pozvani jednom)
// ako kod tebe projectionStarted nikad ne postaje true, vidi tačku 3 ispod!
        bool projActive = projectionStarted;

        // 2) da li su svi izašli (ti već imaš allExited gore iznad)
        bool emptyHall = allExited || people3D.empty();

        // 3) PRE filma: projekcija je aktivna, film nije krenuo i nije završio, a nisu svi izašli
        bool preShow = projActive && (!filmStarted) && (!filmFinished) && (!allExited);

        // SALA svetlo: samo u preShow fazi
        bool hallOn = preShow;

        // PLATNO svetlo: samo dok film traje
        bool screenOn = filmStarted;

        // ako je sala prazna (start ili kraj) -> sve ugasi
        if (emptyHall) {
            hallOn = false;
            // screenOn ostaje false osim ako film traje (ali tada sala nije prazna)
        }


        // 2) enable/disable u shaderu
        glUniform1i(glGetUniformLocation(phongShader, "uLights[0].enabled"), hallOn ? 1 : 0);
        glUniform1i(glGetUniformLocation(phongShader, "uLights[1].enabled"), screenOn ? 1 : 0);
       


        glm::vec3 screenPos = getScreenPos3D();
        glUniform3f(glGetUniformLocation(phongShader, "uLights[1].pos"), screenPos.x, screenPos.y, screenPos.z + 0.1f);

        // phongTexShader per frame setup
        glUseProgram(phongTexShader);
        glUniformMatrix4fv(glGetUniformLocation(phongTexShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(phongTexShader, "uP"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(glGetUniformLocation(phongTexShader, "uViewPos"), 1, glm::value_ptr(camPos));

        glUniform1i(glGetUniformLocation(phongTexShader, "uLights[0].enabled"), hallOn ? 1 : 0);
        glUniform1i(glGetUniformLocation(phongTexShader, "uLights[1].enabled"), screenOn ? 1 : 0);

        glUniform3f(glGetUniformLocation(phongTexShader, "uLights[0].pos"), 0.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(phongTexShader, "uLights[0].kA"), 0.10f, 0.10f, 0.10f);
        glUniform3f(glGetUniformLocation(phongTexShader, "uLights[0].kD"), 0.40f, 0.40f, 0.40f);

        glUniform3f(glGetUniformLocation(phongTexShader, "uLights[0].kS"), 1.0f, 1.0f, 1.0f);

        glUniform3f(glGetUniformLocation(phongTexShader, "uLights[1].pos"), screenPos.x, screenPos.y, screenPos.z + 0.1f);
        glUniform3f(glGetUniformLocation(phongTexShader, "uLights[1].kA"), 0.1f, 0.1f, 0.1f);
        glUniform3f(glGetUniformLocation(phongTexShader, "uLights[1].kD"), 0.4f, 0.4f, 0.4f);
        glUniform3f(glGetUniformLocation(phongTexShader, "uLights[1].kS"), 0.6f, 0.6f, 0.6f);

        glUniform1f(glGetUniformLocation(phongTexShader, "uMaterial.shine"), 64.0f);
        glUniform3f(glGetUniformLocation(phongTexShader, "uMaterial.kA"), 0.2f, 0.2f, 0.2f);
        glUniform3f(glGetUniformLocation(phongTexShader, "uMaterial.kD"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(phongTexShader, "uMaterial.kS"), 1.0f, 1.0f, 1.0f);

        glUniform1i(glGetUniformLocation(phongTexShader, "uTex"), 0);
       // glUniform1f(glGetUniformLocation(phongTexShader, "uEmissive"), 0.15f);
        float emissive = (filmStarted ? 0.25f : 0.0f);
        glUniform1f(glGetUniformLocation(phongTexShader, "uEmissive"), emissive);



        glUseProgram(0);

        drawRoom(phongTexShader, view, proj, camPos);













        // ===== DRAW DOOR (3D) =====
        glUseProgram(phongShader);
        glBindVertexArray(cubeVAO);



        glUniform1f(uMatShineLoc, 24.0f);
        glUniform3f(uMatSLoc, 0.10f, 0.10f, 0.10f);







        // okvir vrata = 4 tanke letvice (hollow frame)
        {
            float frameThickX = 0.06f; // debljina stoka levo/desno
            float frameThickY = 0.06f; // debljina grede gore
            float frameDepth = 0.06f; // malo deblje od krila
            float halfW = gDoorFrameHalf.x;
            float halfH = gDoorFrameHalf.y;

           // glm::vec3 base = gDoorCenter + glm::vec3(0, 0, -0.02f);
            glm::vec3 base = gDoorCenter + glm::vec3(0, 0, -DOOR_GAP_Z);


            auto drawBar = [&](glm::vec3 pos, glm::vec3 half, glm::vec3 col)
                {
                    glm::mat4 M(1.0f);
                    M = glm::translate(M, pos);
                    M = glm::scale(M, half * 2.0f);
                    glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(M));
                    glUniform3f(uMatDLoc, col.x, col.y, col.z);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                };

            glm::vec3 col(0.05f, 0.05f, 0.05f);

            // леви сток
            drawBar(base + glm::vec3(-halfW + frameThickX, 0.0f, 0.0f),
                glm::vec3(frameThickX, halfH, frameDepth), col);

            // десни сток
            drawBar(base + glm::vec3(+halfW - frameThickX, 0.0f, 0.0f),
                glm::vec3(frameThickX, halfH, frameDepth), col);

            // горња греда
            drawBar(base + glm::vec3(0.0f, +halfH - frameThickY, 0.0f),
                glm::vec3(halfW, frameThickY, frameDepth), col);

            // доња греда (ако хоћеш праг; ако не, слободно закоментариши)
            // drawBar(base + glm::vec3(0.0f, -halfH + frameThickY, 0.0f),
            //         glm::vec3(halfW, frameThickY, frameDepth), col);
        }


        // levo krilo (klizi ulevo)
        {
            glm::mat4 M(1.0f);
           // glm::vec3 pos = gDoorCenter + glm::vec3(-doorOffset, 0.0f, 0.0f);

            float open01 = doorOffset / DOOR_MAX;               // 0..1
            float zSlide = (DOOR_OVERLAP_Z + DOOR_GAP_Z) * open01; // kad je zatvoreno = 0, kad je otvoreno = overlap

            glm::vec3 pos = gDoorCenter + glm::vec3(-doorOffset, 0.0f, +zSlide);

            M = glm::translate(M, pos);
            M = glm::scale(M, gDoorHalf * 2.0f);
            glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(M));
            glUniform3f(uMatDLoc, 0.08f, 0.08f, 0.08f);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // desno krilo (klizi udesno)
        {
            glm::mat4 M(1.0f);
           // glm::vec3 pos = gDoorCenter + glm::vec3(+doorOffset, 0.0f, 0.0f);


            float open01 = doorOffset / DOOR_MAX;
            float zSlide = (DOOR_OVERLAP_Z + DOOR_GAP_Z) * open01;

            glm::vec3 pos = gDoorCenter + glm::vec3(+doorOffset, 0.0f, +zSlide);



            M = glm::translate(M, pos);
            M = glm::scale(M, gDoorHalf * 2.0f);
            glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(M));
            glUniform3f(uMatDLoc, 0.08f, 0.08f, 0.08f);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);
        glUseProgram(0);
























        // DRAW STEPS + SEATS
        glUseProgram(phongShader);
        glBindVertexArray(cubeVAO);

        for (const auto& st : steps3D)
        {
            glm::mat4 M(1.0f);
            M = glm::translate(M, st.pos);
            M = glm::scale(M, st.half * 2.0f);
            glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(M));
            glUniform3f(uMatDLoc, 0.18f, 0.18f, 0.18f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        for (const auto& s : seats3D)
        {
            glm::vec3 forward = glm::normalize(glm::vec3(std::sin(s.yaw), 0.0f, std::cos(s.yaw)));

            // baza
            {
                glm::mat4 M(1.0f);
                M = glm::translate(M, s.pos);
                M = glm::rotate(M, s.yaw, glm::vec3(0, 1, 0));

                glm::vec3 baseHalf = glm::vec3(s.half.x, s.half.y * 0.55f, s.half.z * 0.90f);
                M = glm::scale(M, baseHalf * 2.0f);

                glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(M));

                if (s.status == SeatStatus3D::Free) glUniform3f(uMatDLoc, 0.2f, 0.2f, 0.9f);
                else if (s.status == SeatStatus3D::Reserved) glUniform3f(uMatDLoc, 0.95f, 0.85f, 0.1f);
                else glUniform3f(uMatDLoc, 0.9f, 0.1f, 0.1f);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // naslon
            {
                glm::vec3 backOffset = -forward * (s.half.z * 0.85f);
                glm::vec3 posBack = s.pos + backOffset + glm::vec3(0.0f, s.half.y * 0.95f, 0.0f);

                glm::mat4 M(1.0f);
                M = glm::translate(M, posBack);
                M = glm::rotate(M, s.yaw, glm::vec3(0, 1, 0));

                glm::vec3 backHalf = glm::vec3(s.half.x * 1.10f, s.half.y * 1.10f, s.half.z * 0.30f);
                M = glm::scale(M, backHalf * 2.0f);

                glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(M));

                if (s.status == SeatStatus3D::Free) glUniform3f(uMatDLoc, 0.12f, 0.12f, 0.65f);
                else if (s.status == SeatStatus3D::Reserved) glUniform3f(uMatDLoc, 0.75f, 0.65f, 0.08f);
                else glUniform3f(uMatDLoc, 0.65f, 0.08f, 0.08f);

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // DRAW PEOPLE MODELS (OBJ)
       // konstanta koja ispravlja "gde je napred" na tvom OBJ modelu
// ako su leđima ka platnu -> stavi PI, ako su licem -> 0
       // OBJ modeli često "gledaju" u +X, a mi računamo yaw kao da je +Z.
// Zato je najčešći offset -90 stepeni.
        static const float HUMAN_FORWARD_OFFSET = 0.0f;




        // helper: isto kao computeAisleX u People3D.cpp
        auto computeAisleX_local = [&]() -> float {
            float minX = seats3D.empty() ? -3.0f : seats3D[0].pos.x;
            for (auto& s : seats3D) minX = std::min(minX, s.pos.x);
            return minX - 0.9f;
            };

        // helper: isto kao computeEntryPos u People3D.cpp
        auto computeEntryPos_local = [&]() -> glm::vec3 {
            glm::vec3 screen = getScreenPos3D();
            float aisleX = computeAisleX_local();
            float y = 0.2f;
            float z = screen.z + 0.7f;
            return glm::vec3(aisleX, y, z);
            };





        // probaj i 0.0f ako obrne

        for (int i = 0; i < (int)people3D.size(); i++)
        {
            auto& p = people3D[i];
            if (!p.started) continue;

            float yawP = 0.0f;

            if (p.stage == 3 && p.seatIndex >= 0 && p.seatIndex < (int)seats3D.size())
            {
                const Seat3D& s = seats3D[p.seatIndex];
                yawP = s.yaw + HUMAN_FORWARD_OFFSET;
            }

            else
            {// dok hoda: gledaj u smer kretanja, ali za izlazak koristi "sledeću tačku"
                glm::vec3 next = p.target;

                // stage 4: ide ka prolazu (aisle X)
                if (p.stage == 4) {
                    float aisleX = computeAisleX_local();
                    next = glm::vec3(aisleX, p.pos.y, p.pos.z);
                }
                // stage 5: ide ka izlazu (entry Z)
                else if (p.stage == 5) {
                    glm::vec3 entry = computeEntryPos_local();
                    next = glm::vec3(p.pos.x, p.pos.y, entry.z);
                }

                glm::vec3 dir = next - p.pos;
                dir.y = 0.0f;

                if (glm::length(dir) > 0.0001f)
                {
                    dir = glm::normalize(dir);
                    yawP = std::atan2(dir.x, dir.z) + HUMAN_FORWARD_OFFSET;
                }

            }


            glm::vec3 drawPos = p.pos;

            // ako sedi, spusti ga i gurni ka naslonu
            if (p.stage == 3 && p.seatIndex >= 0 && p.seatIndex < (int)seats3D.size())
            {
                const Seat3D& s = seats3D[p.seatIndex];
                glm::vec3 fwd = glm::normalize(glm::vec3(std::sin(s.yaw), 0.0f, std::cos(s.yaw)));

                drawPos = p.target;

                drawPos += (+fwd) * (s.half.z * 0.10f);
                // mikro korekcije (podesi po osećaju)
                drawPos += glm::vec3(0.0f, 0.08f, 0.0f);          // PODIGNI malo
                drawPos += (-fwd) * (s.half.z * 0.05f);          // malo KA NASLONU (ili smanji ako upada)
            }

            glm::mat4 M(1.0f);
            M = glm::translate(M, drawPos);
            M = glm::rotate(M, yawP, glm::vec3(0, 1, 0));

            if (p.stage == 3) {
                float sitPitch = glm::radians(-12.0f);   // probaj -8 do -18
                M = glm::rotate(M, sitPitch, glm::vec3(1, 0, 0)); // nagni unazad
            }

            M = glm::scale(M, glm::vec3(0.45f));
            

            if (humans.empty()) continue;
            int idx = p.modelIndex % (int)humans.size();

            glUseProgram(phongTexShader);
            glUniformMatrix4fv(glGetUniformLocation(phongTexShader, "uM"), 1, GL_FALSE, glm::value_ptr(M));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, humans[idx]->getTexture());
            humans[idx]->Draw();

            glBindTexture(GL_TEXTURE_2D, 0);
            glUseProgram(0);
        }


        bool prevDepth = (glIsEnabled(GL_DEPTH_TEST) == GL_TRUE);

        glDisable(GL_DEPTH_TEST);
        drawMeBottomLeft(window, nameTex);
        drawCrosshair();

        if (prevDepth) glEnable(GL_DEPTH_TEST);
        else           glDisable(GL_DEPTH_TEST);


        glfwSwapBuffers(window);
        glfwPollEvents();

        double dt = glfwGetTime() - start;
        if (dt < FRAME_TIME)
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(FRAME_TIME - dt));
        }
    }

    glfwTerminate();
    return 0;
}
