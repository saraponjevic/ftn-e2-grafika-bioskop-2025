#include "Seat3D.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_inverse.hpp>

std::vector<Seat3D> seats3D;
std::vector<Step3D> steps3D;

// ===== PODESAVANJA RASPOREDA =====
static const int NUM_ROWS = 6;
static const int NUM_COLS = 14;

static const glm::vec3 SEAT_HALF(0.25f, 0.25f, 0.25f);

static const float COL_GAP_X = 0.70f;
static const float ROW_GAP_Z = 0.95f;
static const float STEP_H = 0.22f;

// PRVI RED = NAJDALJI (najbliži platnu)
static const glm::vec3 SEATS_ORIGIN(0.0f, 0.35f, -3.20f);

// centar platna (da stolice gledaju ka njemu)
static const glm::vec3 SCREEN_POS(0.0f, 1.30f, -5.20f);

// platforma stepenika
static const float STEP_DEPTH_Z = 0.55f;

glm::vec3 getScreenPos3D() { return SCREEN_POS; }

// ===== RAYCAST =====
struct Ray { glm::vec3 o, d; };

static Ray makeRayFromNDC(float ndcX, float ndcY, const glm::mat4& view, const glm::mat4& proj)
{
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 pNear = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 pFar = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

    pNear /= pNear.w;
    pFar /= pFar.w;

    Ray r;
    r.o = glm::vec3(pNear);
    r.d = glm::normalize(glm::vec3(pFar - pNear));
    return r;
}

static bool rayAABB(const Ray& r, const glm::vec3& bmin, const glm::vec3& bmax, float& tHit)
{
    float tmin = 0.0f;
    float tmax = 1e9f;

    for (int i = 0; i < 3; i++)
    {
        float o = r.o[i];
        float d = r.d[i];

        if (std::fabs(d) < 1e-6f)
        {
            if (o < bmin[i] || o > bmax[i]) return false;
        }
        else
        {
            float invD = 1.0f / d;
            float t0 = (bmin[i] - o) * invD;
            float t1 = (bmax[i] - o) * invD;
            if (t0 > t1) std::swap(t0, t1);

            tmin = std::max(tmin, t0);
            tmax = std::min(tmax, t1);
            if (tmin > tmax) return false;
        }
    }

    tHit = tmin;
    return true;
}

// ===== GENERISANJE =====
void initSeats3D()
{
    seats3D.clear();
    steps3D.clear();

    seats3D.reserve(NUM_ROWS * NUM_COLS);
    steps3D.reserve(NUM_ROWS);

    float totalW = (NUM_COLS - 1) * COL_GAP_X;
    float x0 = -totalW * 0.5f;

    for (int r = 0; r < NUM_ROWS; r++)
    {
        float yRow = SEATS_ORIGIN.y + r * STEP_H;
        float zRow = SEATS_ORIGIN.z + r * ROW_GAP_Z;

        Step3D st;
        st.pos = glm::vec3(0.0f, (r * STEP_H) * 0.5f, zRow - (ROW_GAP_Z * 0.35f));
        st.half = glm::vec3((NUM_COLS * COL_GAP_X) * 0.55f, (STEP_H * 0.5f), STEP_DEPTH_Z);
        steps3D.push_back(st);

        for (int c = 0; c < NUM_COLS; c++)
        {
            float x = x0 + c * COL_GAP_X;

            Seat3D s;
            s.row = r;
            s.col = c;
            s.status = SeatStatus3D::Free;

            s.pos = glm::vec3(x, yRow, zRow);
            s.half = SEAT_HALF;

            glm::vec3 toScreen = glm::normalize(SCREEN_POS - s.pos);
            s.yaw = std::atan2(toScreen.x, toScreen.z);

            seats3D.push_back(s);
        }
    }
}

// ===== PICKING =====
int pickSeat3D(float ndcX, float ndcY, const glm::mat4& view, const glm::mat4& proj)
{
    Ray r = makeRayFromNDC(ndcX, ndcY, view, proj);

    int best = -1;
    float bestT = 1e9f;

    for (int i = 0; i < (int)seats3D.size(); i++)
    {
        const Seat3D& s = seats3D[i];

        glm::vec3 bmin = s.pos - s.half;
        glm::vec3 bmax = s.pos + s.half;

        float t;
        if (rayAABB(r, bmin, bmax, t))
        {
            if (t < bestT) { bestT = t; best = i; }
        }
    }
    return best;
}

void toggleReserveSeat3D(int idx)
{
    if (idx < 0 || idx >= (int)seats3D.size()) return;

    Seat3D& s = seats3D[idx];
    if (s.status == SeatStatus3D::Free) s.status = SeatStatus3D::Reserved;
    else if (s.status == SeatStatus3D::Reserved) s.status = SeatStatus3D::Free;
}

// ===== BUY =====
void buySeats3D(int N)
{
    if (N <= 0) return;

    for (int r = 0; r < NUM_ROWS; r++)
    {
        int count = 0;
        int startCol = -1;

        for (int c = NUM_COLS - 1; c >= 0; c--)
        {
            Seat3D& s = seats3D[r * NUM_COLS + c];

            if (s.status == SeatStatus3D::Free)
            {
                if (count == 0) startCol = c;
                count++;

                if (count == N)
                {
                    for (int k = 0; k < N; k++)
                        seats3D[r * NUM_COLS + (startCol - k)].status = SeatStatus3D::Bought;
                    return;
                }
            }
            else
            {
                count = 0;
                startCol = -1;
            }
        }
    }
}
