#pragma once
#include <vector>

enum class SeatStatus {
    Free,
    Reserved,
    Bought
};

extern const int   NUM_ROWS;
extern const int   NUM_COLS;
extern const float SEAT_W;
extern const float SEAT_H;


struct Seat {
    float x, y;
    SeatStatus status;
};

extern std::vector<Seat> seats;

void initSeats();
void handleSeatClick(float ndcX, float ndcY);
void buySeats(int N);
float seatScaleForRow(int row);

