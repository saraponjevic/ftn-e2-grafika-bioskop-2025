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


// pravljenje sedista - njihove x i y pozicije + velicina + pocetni status
void initSeats()
{
    seats.clear();

    //SEAT_W i SEAT_H su osnovna širina i visina sedišta
    float baseW = SEAT_W;
    float baseH = SEAT_H;

    float yStart = -0.75f;  //Početna Y pozicija donjeg reda sedišta.

    float maxScale = seatScaleForRow(0);
    float rowSpacing = baseH * maxScale * 0.70f;   //razmak između redova po Y osi

    for (int r = 0; r < NUM_ROWS; r++)
    {
        float scale = seatScaleForRow(r);
        float seatWRow = baseW * scale;    

        float stepX = seatWRow * 0.6f;   //stepX = horizontalni razmak između sedišta
        float totalWidth = (NUM_COLS - 1) * stepX;   //totalWidth = širina cele grupe stolica u tom redu.
        float startX = -totalWidth / 2.0f;   //startX = x koordinata prvog sedišta u redu, tako da su sedišta centrirana oko x = 0

        float y = yStart + r * rowSpacing;   //Y koordinata za ceo red r.

        for (int c = 0; c < NUM_COLS; c++)   //c = kolona (broj sedišta u redu)
        {
            Seat s;
            s.x = startX + c * stepX;  //s.x = pozicija sedišta po X osi
            s.y = y;   //s.y = ista za sva sedišta u redu.
            s.status = SeatStatus::Free;   //status = Free → na početku su sva sedišta slobodna (plava

            seats.push_back(s);
        }
    }
}


// klik na sedista
void handleSeatClick(float ndcX, float ndcY)   
{
    for (int i = 0; i < (int)seats.size(); i++)
    {
        Seat& s = seats[i];   

        int row = i / NUM_COLS;  //Iz indeksa i izvlačiš red
        float scale = seatScaleForRow(row);

        float seatWidth = SEAT_W * scale;
        float seatHeight = SEAT_H * scale;


        //Da li se klik nalazi unutar pravougaonika sedišta
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

// kupovina karata 
void buySeats(int N)
{
   // for (int r = NUM_ROWS - 1; r >= 0; r--)
    for (int r = 0; r < NUM_ROWS; r++)

    {
        int count = 0;  //koliko si trenutno uzastopnih slobodnih sedišta
        int startCol = -1;   //kolona gde je počela trenutna grupa slobodnih sedišta

        for (int c = NUM_COLS - 1; c >= 0; c--)   //krećeš od desnog kraja reda
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
            {           //Ako sedište nije Free
                count = 0;  //resetuješ count na 0
                startCol = -1;  
            }
        }
    }
}
