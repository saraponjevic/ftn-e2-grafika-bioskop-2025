// ============================
// Room.cpp
// ============================
#include "Room.h"
#include "Util.h"

#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>

#include <vector>
#include <iostream>

// ====== interni resursi ======
static unsigned int roomVAO = 0, roomVBO = 0;

// teksture
static unsigned int texFloor = 0;
static unsigned int texWall = 0;
static unsigned int texCeiling = 0;
static unsigned int texScreen = 0;

unsigned int getRoomTexFloor() { return texFloor; }
unsigned int getRoomTexWall() { return texWall; }
unsigned int getRoomTexCeiling() { return texCeiling; }
unsigned int getRoomTexScreen() { return texScreen; }

static RoomDims dims = { 10.0f, 4.0f, 14.0f };
RoomDims getRoomDims() { return dims; }


// Vertex: pos(3) + normal(3) + uv(2)
struct Vtx
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

static void pushQuad(std::vector<Vtx>& out,
    glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
    glm::vec3 n,
    glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3)
{
    out.push_back({ p0.x,p0.y,p0.z, n.x,n.y,n.z, uv0.x,uv0.y });
    out.push_back({ p1.x,p1.y,p1.z, n.x,n.y,n.z, uv1.x,uv1.y });
    out.push_back({ p2.x,p2.y,p2.z, n.x,n.y,n.z, uv2.x,uv2.y });

    out.push_back({ p0.x,p0.y,p0.z, n.x,n.y,n.z, uv0.x,uv0.y });
    out.push_back({ p2.x,p2.y,p2.z, n.x,n.y,n.z, uv2.x,uv2.y });
    out.push_back({ p3.x,p3.y,p3.z, n.x,n.y,n.z, uv3.x,uv3.y });
}

void setScreenTexture(unsigned int tex)
{
    texScreen = tex;
}

void initRoom()
{
    std::cout << "[initRoom] start\n";

    texFloor = loadImageToTexture("res/floor.jpeg");
    texWall = loadImageToTexture("res/wall.jpeg");
    texCeiling = loadImageToTexture("res/ceiling.jpeg");
    texScreen = loadImageToTexture("res/scrr.jpg");

    printf("texFloor=%u texWall=%u texCeiling=%u texScreen=%u\n",
        texFloor, texWall, texCeiling, texScreen);

    float W = dims.W;
    float H = dims.H;
    float D = dims.D;

    float x0 = -W * 0.5f, x1 = W * 0.5f;
    float y0 = 0.0f, y1 = H;
    float z0 = -D * 0.5f, z1 = D * 0.5f;

    std::vector<Vtx> v;

    float tileFloor = 6.0f;
    float tileWall = 3.0f;
    float tileCeil = 4.0f;

    // POD
    pushQuad(v,
        { x0,y0,z1 }, { x1,y0,z1 }, { x1,y0,z0 }, { x0,y0,z0 },
        { 0,1,0 },
        { 0,0 }, { tileFloor,0 }, { tileFloor,tileFloor }, { 0,tileFloor }
    );

    // PLAFON
    pushQuad(v,
        { x0,y1,z0 }, { x1,y1,z0 }, { x1,y1,z1 }, { x0,y1,z1 },
        { 0,-1,0 },
        { 0,0 }, { tileCeil,0 }, { tileCeil,tileCeil }, { 0,tileCeil }
    );

    // BACK wall (z0) sa rupom za vrata (kod platna)
    float doorW = 1.7f;
    float doorH = 2.2f;
    float doorCX = -W * 0.45f;

    float doorL = doorCX - doorW * 0.5f;
    float doorR = doorCX + doorW * 0.5f;
    float doorB = y0;        // od poda
    float doorT = doorH;     // do visine vrata

    // clamp radi sigurnosti
    doorL = glm::max(doorL, x0 + 0.01f);
    doorR = glm::min(doorR, x1 - 0.01f);
    doorT = glm::min(doorT, y1 - 0.01f);

    // 1) levi deo BACK zida
    pushQuad(v,
        { x0, y0, z0 }, { doorL, y0, z0 }, { doorL, y1, z0 }, { x0, y1, z0 },
        { 0,0, 1 },
        { 0,0 }, { tileWall,0 }, { tileWall,tileWall }, { 0,tileWall }
    );

    // 2) desni deo BACK zida
    pushQuad(v,
        { doorR, y0, z0 }, { x1, y0, z0 }, { x1, y1, z0 }, { doorR, y1, z0 },
        { 0,0, 1 },
        { 0,0 }, { tileWall,0 }, { tileWall,tileWall }, { 0,tileWall }
    );

    // 3) gornji deo BACK zida (iznad vrata)
    pushQuad(v,
        { doorL, doorT, z0 }, { doorR, doorT, z0 }, { doorR, y1, z0 }, { doorL, y1, z0 },
        { 0,0, 1 },
        { 0,0 }, { 1,0 }, { 1,1 }, { 0,1 }
    );


    // FRONT wall (z1)
    pushQuad(v,
        { x1,y0,z1 }, { x0,y0,z1 }, { x0,y1,z1 }, { x1,y1,z1 },
        { 0,0,-1 },
        { 0,0 }, { tileWall,0 }, { tileWall,tileWall }, { 0,tileWall }
    );



    // LEFT wall (x0)
    pushQuad(v,
        { x0,y0,z1 }, { x0,y0,z0 }, { x0,y1,z0 }, { x0,y1,z1 },
        { 1,0,0 },
        { 0,0 }, { tileWall,0 }, { tileWall,tileWall }, { 0,tileWall }
    );

    // RIGHT wall (x1)
    pushQuad(v,
        { x1,y0,z0 }, { x1,y0,z1 }, { x1,y1,z1 }, { x1,y1,z0 },
        { -1,0,0 },
        { 0,0 }, { tileWall,0 }, { tileWall,tileWall }, { 0,tileWall }
    );

    // PLATNO (na z0)
    float screenW = W * 0.55f;
    float screenH = H * 0.45f;

    float sx0 = -screenW * 0.5f;
    float sx1 = screenW * 0.5f;
    float sy0 = H * 0.45f;
    float sy1 = sy0 + screenH;

    float sz = z0 + 0.02f;

    pushQuad(v,
        { sx0,sy0,sz }, { sx1,sy0,sz }, { sx1,sy1,sz }, { sx0,sy1,sz },
        { 0,0, 1 },
        { 0,0 }, { 1,0 }, { 1,1 }, { 0,1 }
    );

    // VAO/VBO
    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);

    glBindVertexArray(roomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vtx), v.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vtx), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vtx), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vtx), (void*)(6 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void setTexture(unsigned int shader, unsigned int tex)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(shader, "uTex"), 0);
}

void drawRoom(unsigned int phongTexShader, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos)
{
    if (!roomVAO) return;

    glUseProgram(phongTexShader);

    int uTilingLoc = glGetUniformLocation(phongTexShader, "uTiling");
    if (uTilingLoc != -1) glUniform1f(uTilingLoc, 1.0f);

    int uMLoc = glGetUniformLocation(phongTexShader, "uM");
    int uVLoc = glGetUniformLocation(phongTexShader, "uV");
    int uPLoc = glGetUniformLocation(phongTexShader, "uP");
    int uViewPosLoc = glGetUniformLocation(phongTexShader, "uViewPos");

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(uMLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uVLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uPLoc, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(uViewPosLoc, 1, glm::value_ptr(camPos));

    glBindVertexArray(roomVAO);

    // POD
    setTexture(phongTexShader, texFloor);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // PLAFON
    setTexture(phongTexShader, texCeiling);
    glDrawArrays(GL_TRIANGLES, 6, 6);

    // ZIDOVI (sada = 36)
    setTexture(phongTexShader, texWall);
    glDrawArrays(GL_TRIANGLES, 12, 36);

    // PLATNO (pomereno posle zidova)
    setTexture(phongTexShader, texScreen);
    glDrawArrays(GL_TRIANGLES, 48, 6);


    glBindVertexArray(0);
    glUseProgram(0);
}
