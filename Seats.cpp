#include "Seats.h"
#include <vector>
#include <cmath>
#include <GLFW/glfw3.h>


std::vector<Seat> seats;

const int NUM_ROWS = 6;
const int NUM_COLS = 14;

const float SEAT_W = 0.17f;   
const float SEAT_H = 0.17f;  


float seatScaleForRow(int row)
{
    switch (row) {
    case 0: return 1.20f;  // najvece- najblize nama
    case 1: return 1.10f;
    case 2: return 1.00f;
    case 3: return 0.90f;
    case 4: return 0.80f;
    case 5: return 0.75f;
    default: return 1.0f;
    }
}


void initSeats()
{
    seats.clear();

    float baseW = SEAT_W;
    float baseH = SEAT_H;

    float yStart = -0.75f;

    float maxScale = seatScaleForRow(0);
    float rowSpacing = baseH * maxScale * 0.70f;

    for (int r = 0; r < NUM_ROWS; r++)
    {
        float scale = seatScaleForRow(r);
        float seatWRow = baseW * scale;

        float stepX = seatWRow * 0.6f;
        float totalWidth = (NUM_COLS - 1) * stepX;
        float startX = -totalWidth / 2.0f;

        float y = yStart + r * rowSpacing;

        for (int c = 0; c < NUM_COLS; c++)
        {
            Seat s;
            s.x = startX + c * stepX;
            s.y = y;
            s.status = SeatStatus::Free;

            seats.push_back(s);
        }
    }
}


void handleSeatClick(float ndcX, float ndcY)
{
    for (int i = 0; i < (int)seats.size(); i++)
    {
        Seat& s = seats[i];

        int row = i / NUM_COLS;
        float scale = seatScaleForRow(row);

        float seatWidth = SEAT_W * scale;
        float seatHeight = SEAT_H * scale;

        if (ndcX > s.x - seatWidth / 2 && ndcX < s.x + seatWidth / 2 &&
            ndcY > s.y - seatHeight / 2 && ndcY < s.y + seatHeight / 2)
        {
            if (s.status == SeatStatus::Free)
                s.status = SeatStatus::Reserved;
            else if (s.status == SeatStatus::Reserved)
                s.status = SeatStatus::Free;

            return; 
        }
    }
}


void buySeats(int N)
{
    for (int r = NUM_ROWS - 1; r >= 0; r--)
    {
        int count = 0;
        int startCol = -1;

        for (int c = NUM_COLS - 1; c >= 0; c--)
        {
            Seat& s = seats[r * NUM_COLS + c];

            if (s.status == SeatStatus::Free)
            {
                if (count == 0) startCol = c;
                count++;

                if (count == N)
                {
                    for (int k = 0; k < N; k++)
                        seats[r * NUM_COLS + (startCol - k)].status = SeatStatus::Bought;
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
