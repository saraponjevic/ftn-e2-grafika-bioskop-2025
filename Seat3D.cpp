#include "Seat3D.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/constants.hpp>   // za pi

std::vector<Seat3D> seats3D;
std::vector<Step3D> steps3D;

// ===== RASPORED =====
static const int NUM_ROWS = 8;      // bilo 6 -> sad veæa tribina
static const int NUM_COLS = 14;

// Sedište (AABB half-size) — malo veæe da se lepše vidi
static const glm::vec3 SEAT_HALF(0.1f, 0.14f, 0.18f);

// ŠIRINA + DUBINA tribine
static const float COL_GAP_X = 0.42f;  // šire po X -> zauzima više sale
static const float ROW_GAP_Z = 0.70f;  // dublje po Z -> redovi dalje

// Stepenik po redu (prirodnije)
static const float STEP_H = 0.16f; // visina “koraka”
static const float STEP_THICK = 0.10f; // debljina platforme

// Pomeri tribinu ka centru sale da izgleda “veæe”
static const glm::vec3 SEATS_ORIGIN(0.0f, 0.10f, -1.20f);

// Platno
static const glm::vec3 SCREEN_POS(0.0f, 1.55f, -6.10f);
glm::vec3 getScreenPos3D() { return SCREEN_POS; }

// NEMA boènih prolaza -> maksimalna širina
static const int SIDE_AISLE_COLS = 0;

// Boèni prolaz sa leve i desne strane (prazno)
static const float AISLE_X = 0.90f;   // probaj 0.7f - 1.2f

// Pola širine sale (od centra do zida) — privremeno.
// Kasnije æemo vezati za Room dimenziju.
static const float HALF_ROOM_X = 5.0f;


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

    const int firstSeatCol = SIDE_AISLE_COLS;
    const int lastSeatCol = NUM_COLS - 1 - SIDE_AISLE_COLS;
    const int seatColsCount = (lastSeatCol - firstSeatCol + 1);

    seats3D.reserve(NUM_ROWS * seatColsCount);
    steps3D.reserve(NUM_ROWS);

    // centriranje po X po “pravim” sedištima
    float gapX = COL_GAP_X;
    float totalW = (seatColsCount - 1) * gapX;

    float maxHalfSeats = HALF_ROOM_X - AISLE_X;
    if (totalW * 0.5f > maxHalfSeats) {
        gapX = (maxHalfSeats * 2.0f) / (seatColsCount - 1);
        totalW = (seatColsCount - 1) * gapX;
    }

    float x0 = -totalW * 0.5f;


    // Maks polu-širina sedišta da ostane prolaz sa strane
   // float maxHalfSeats = HALF_ROOM_X - AISLE_X;

    // Ako je raspored sedišta preširok — smanji COL_GAP_X automatski (bez menjanja NUM_COLS)
    if (totalW * 0.5f > maxHalfSeats) {
        float newGap = (maxHalfSeats * 2.0f) / (seatColsCount - 1);
        // napomena: COL_GAP_X je const, pa ne možemo direktno da je menjamo
        // zato æemo koristiti lokalni gapX umesto COL_GAP_X
    }


    for (int r = 0; r < NUM_ROWS; r++)
    {
        float stepTopY = SEATS_ORIGIN.y + r * STEP_H;
        float zRow = SEATS_ORIGIN.z + r * ROW_GAP_Z;


        // platforma (step)
        Step3D st;
        st.pos = glm::vec3(0.0f, stepTopY - STEP_THICK * 0.5f, zRow);

        st.half = glm::vec3(
            HALF_ROOM_X,               // <-- OD ZIDA DO ZIDA
            STEP_THICK * 0.5f,
            (ROW_GAP_Z * 0.55f)
        );

        steps3D.push_back(st);

        // DODATNA platforma iza poslednjeg reda (razmak do zida)
        {
            float stepTopY = SEATS_ORIGIN.y + (NUM_ROWS)*STEP_H;
            float zRow = SEATS_ORIGIN.z + (NUM_ROWS)*ROW_GAP_Z;

            Step3D st;
            st.pos = glm::vec3(0.0f, stepTopY - STEP_THICK * 0.5f, zRow);
            st.half = glm::vec3(
                HALF_ROOM_X,
                STEP_THICK * 0.5f,
                (ROW_GAP_Z * 0.55f)
            );
            steps3D.push_back(st);
        }


        // sedišta
        for (int c = firstSeatCol; c <= lastSeatCol; c++)
        {
            int idxInRow = c - firstSeatCol;
            float x = x0 + idxInRow * gapX;


            Seat3D s;
            s.row = r;
            s.col = c;
            s.status = SeatStatus3D::Free;
            s.half = SEAT_HALF;

            float seatCenterY = stepTopY + s.half.y + 0.012f;

            // sedište malo ka "napred" na platformi (da ne stoji taèno na sredini)
            float seatZ = zRow + 0.18f;

            s.pos = glm::vec3(x, seatCenterY, seatZ);

            // >>> OKRETANJE KA PLATNU <<<
            glm::vec3 toScreen = glm::normalize(SCREEN_POS - s.pos);

            // yaw koji ti je bio je ok, ali pošto su kocke simetriène,
            // okrenemo ga za 180° da “prednja strana” gleda ka platnu.
           // s.yaw = std::atan2(toScreen.x, toScreen.z) + glm::pi<float>();
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
        // --- PICK BOX treba da pokrije BAZU + NASLON ---
        glm::vec3 baseHalf = glm::vec3(s.half.x, s.half.y * 0.55f, s.half.z * 0.90f);
        glm::vec3 baseMin = s.pos - baseHalf;
        glm::vec3 baseMax = s.pos + baseHalf;

        // forward iz yaw (isto kao u crtanju)
        glm::vec3 forward = glm::normalize(glm::vec3(std::sin(s.yaw), 0.0f, std::cos(s.yaw)));

        // naslon je "iza" (suprotno od forward)
        glm::vec3 backOffset = -forward * (s.half.z * 0.85f);
        glm::vec3 posBack = s.pos + backOffset + glm::vec3(0.0f, s.half.y * 0.95f, 0.0f);

        glm::vec3 backHalf = glm::vec3(s.half.x * 1.10f, s.half.y * 1.10f, s.half.z * 0.30f);
        glm::vec3 backMin = posBack - backHalf;
        glm::vec3 backMax = posBack + backHalf;

        // union AABB (obuhvati i bazu i naslon)
        glm::vec3 bmin = glm::min(baseMin, backMin);
        glm::vec3 bmax = glm::max(baseMax, backMax);


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

float getStepsStartZ3D()
{
    if (steps3D.empty()) return SEATS_ORIGIN.z; // fallback
    // steps3D[0] je prvi koji guraš u initSeats3D() (r=0)
    // poèetak stepenika po Z je (pos.z - half.z)
    return steps3D[0].pos.z - steps3D[0].half.z;
}


// ===== BUY =====
void buySeats3D(int N)
{
    if (N <= 0) return;

    // Kreni od POSLEDNJEG reda (pozadi) ka prvom (ka platnu)
    for (int r = NUM_ROWS - 1; r >= 0; r--)
    {
        std::vector<int> idxs;
        idxs.reserve(NUM_COLS);

        // pokupi sva sedista u tom redu
        for (int i = 0; i < (int)seats3D.size(); i++)
            if (seats3D[i].row == r) idxs.push_back(i);

        // sortiraj po koloni opadajuce (najdesnije prvo)
        std::sort(idxs.begin(), idxs.end(), [&](int a, int b) {
            return seats3D[a].col > seats3D[b].col;
            });

        int count = 0;
        int startIdxInIdxs = -1;
        int prevCol = 999999;

        for (int k = 0; k < (int)idxs.size(); k++)
        {
            Seat3D& s = seats3D[idxs[k]];

            // mora biti slobodno + mora biti "susedno" u koloni (u odnosu na prethodno)
            bool adjacentOK = (count == 0) || (s.col == prevCol - 1);

            if (s.status == SeatStatus3D::Free && adjacentOK)
            {
                if (count == 0) startIdxInIdxs = k;
                count++;
                prevCol = s.col;

                if (count == N)
                {
                    for (int t = 0; t < N; t++)
                        seats3D[idxs[startIdxInIdxs + t]].status = SeatStatus3D::Bought;
                    return; // PRVI nadjeni blok -> kupi i izlaz
                }
            }
            else
            {
                // reset bloka
                count = 0;
                startIdxInIdxs = -1;
                prevCol = 999999;

                // ali ako je ovo sediste slobodno, moze biti start novog bloka
                if (s.status == SeatStatus3D::Free)
                {
                    startIdxInIdxs = k;
                    count = 1;
                    prevCol = s.col;

                    if (N == 1)
                    {
                        seats3D[idxs[startIdxInIdxs]].status = SeatStatus3D::Bought;
                        return;
                    }
                }
            }
        }
    }
}
